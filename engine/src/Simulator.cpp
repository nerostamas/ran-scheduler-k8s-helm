#include "ran/Simulator.hpp"

#include <algorithm>

#include "ran/metrics/Kpi.hpp"
#include "ran/phy/LinkAdaptation.hpp"

namespace ran {
namespace {

/// Block error probability as a function of CQI and HARQ attempt. Higher CQI
/// and later attempts (soft combining) lower the error rate. Simplified model.
double blockErrorProbability(Cqi cqi, int attempt) noexcept {
    const double base = 0.9 - 0.055 * static_cast<double>(cqi);  // ~0.9 .. ~0.08
    const double clamped = std::clamp(base, 0.02, 0.9);
    // Each retransmission roughly halves the residual error (soft combining).
    double bler = clamped;
    for (int i = 1; i < attempt; ++i) {
        bler *= 0.5;
    }
    return std::clamp(bler, 0.0, 1.0);
}

}  // namespace

Simulator::Simulator(SimConfig config)
    : config_(config),
      rng_(config.seed),
      scheduler_(mac::makeScheduler(config.scheduler)) {
    ues_.reserve(static_cast<std::size_t>(std::max(1, config_.numUes)));
    // Spread mean CQI across UEs to emulate cell-center vs cell-edge users.
    std::uniform_int_distribution<int> cqiDist(3, 14);
    for (int i = 0; i < config_.numUes; ++i) {
        const auto meanCqi = static_cast<Cqi>(cqiDist(rng_));
        ues_.emplace_back(static_cast<UeId>(i + 1), meanCqi);
    }
}

SimResult Simulator::run() {
    std::uniform_real_distribution<double> unit(0.0, 1.0);

    std::uint64_t totalTransmissions = 0;
    std::uint64_t retransmissions = 0;

    for (Tti tti = 0; tti < config_.numTtis; ++tti) {
        // L1: refresh every UE's channel quality for this TTI.
        for (auto& ue : ues_) {
            ue.updateCqi(rng_);
        }

        // L2: build the candidate set (data queued AND a free HARQ process).
        std::vector<mac::UserEquipment*> candidates;
        candidates.reserve(ues_.size());
        for (auto& ue : ues_) {
            if (ue.bufferSize() > 0 && ue.findFreeHarq() != nullptr) {
                candidates.push_back(&ue);
            }
        }

        const auto decision = scheduler_->schedule(candidates, config_.resourceBlocks);

        double grantedBitsThisTti = 0.0;
        if (decision.granted) {
            auto it = std::find_if(ues_.begin(), ues_.end(), [&](const auto& ue) {
                return ue.id() == decision.ueId;
            });
            if (it != ues_.end()) {
                mac::UserEquipment& ue = *it;

                // L1 link adaptation: size the transport block for this CQI/RB count.
                const Bits tbs = phy::LinkAdaptation::transportBlockSize(
                    ue.currentCqi(), config_.resourceBlocks);
                const Bits toSend = std::min(tbs, ue.bufferSize());

                if (toSend > 0) {
                    mac::HarqProcess* harq = ue.findFreeHarq();
                    harq->startNewTransmission(toSend);
                    ue.dequeueData(toSend);
                    ++totalTransmissions;

                    // HARQ feedback loop (resolved within the TTI for simplicity).
                    bool delivered = false;
                    while (true) {
                        const double bler =
                            blockErrorProbability(ue.currentCqi(), harq->attempt());
                        if (unit(rng_) >= bler) {
                            harq->onAck();
                            delivered = true;
                            break;
                        }
                        if (harq->attempt() > 1) {
                            ++retransmissions;
                        }
                        ++totalTransmissions;
                        if (!harq->onNack()) {
                            break;  // block dropped after max attempts
                        }
                        ++retransmissions;
                    }

                    if (delivered) {
                        ue.recordDelivered(toSend);
                        grantedBitsThisTti = static_cast<double>(toSend);
                    }
                }
            }
        }

        // Update PF averages for every UE (served UE gets its bits, others get 0).
        for (auto& ue : ues_) {
            const double bits =
                (decision.granted && ue.id() == decision.ueId) ? grantedBitsThisTti : 0.0;
            ue.updateEwmaThroughput(bits);
        }
    }

    // Aggregate KPIs.
    SimResult result;
    result.perUeThroughputMbps.reserve(ues_.size());
    for (const auto& ue : ues_) {
        result.perUeThroughputMbps.push_back(ue.averageThroughput(config_.numTtis) / 1e6);
    }
    result.cellThroughputMbps = metrics::sum(result.perUeThroughputMbps);
    result.jainsFairness = metrics::jainsFairnessIndex(result.perUeThroughputMbps);
    result.harqRetxRate =
        totalTransmissions == 0
            ? 0.0
            : static_cast<double>(retransmissions) / static_cast<double>(totalTransmissions);
    return result;
}

}  // namespace ran
