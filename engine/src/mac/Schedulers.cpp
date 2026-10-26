#include <algorithm>
#include <stdexcept>

#include "ran/mac/Scheduler.hpp"
#include "ran/mac/UserEquipment.hpp"
#include "ran/phy/LinkAdaptation.hpp"

namespace ran::mac {
namespace {

/// Round Robin: cycle through UEs so each gets an equal share of grants,
/// independent of channel quality. Fair in time, not throughput-optimal.
class RoundRobinScheduler final : public Scheduler {
public:
    SchedulingDecision schedule(const std::vector<UserEquipment*>& candidates,
                                int /*resourceBlocks*/) const override {
        if (candidates.empty()) {
            return {};
        }
        // Pick the candidate whose id follows lastServed_ in circular order.
        const UserEquipment* best = nullptr;
        UeId bestKey = 0;
        for (const auto* ue : candidates) {
            const UeId key = ue->id() > lastServed_ ? ue->id() - lastServed_
                                                    : ue->id() + kWrap - lastServed_;
            if (best == nullptr || key < bestKey) {
                best = ue;
                bestKey = key;
            }
        }
        lastServed_ = best->id();
        return {true, best->id()};
    }

    std::string_view name() const noexcept override { return "RoundRobin"; }

private:
    static constexpr UeId kWrap = 1u << 24;
    mutable UeId lastServed_ = 0;
};

/// Max C/I: always serve the UE with the best instantaneous channel (CQI),
/// maximizing cell throughput at the cost of cell-edge fairness.
class MaxCiScheduler final : public Scheduler {
public:
    SchedulingDecision schedule(const std::vector<UserEquipment*>& candidates,
                                int /*resourceBlocks*/) const override {
        const UserEquipment* best = nullptr;
        for (const auto* ue : candidates) {
            if (best == nullptr || ue->currentCqi() > best->currentCqi()) {
                best = ue;
            }
        }
        return best ? SchedulingDecision{true, best->id()} : SchedulingDecision{};
    }

    std::string_view name() const noexcept override { return "MaxC/I"; }
};

/// Proportional Fair: serve the UE maximizing achievable-rate / average-rate,
/// balancing throughput and fairness. The industry-standard baseline.
class ProportionalFairScheduler final : public Scheduler {
public:
    SchedulingDecision schedule(const std::vector<UserEquipment*>& candidates,
                                int resourceBlocks) const override {
        const UserEquipment* best = nullptr;
        double bestMetric = -1.0;
        for (const auto* ue : candidates) {
            const double achievable = static_cast<double>(
                phy::LinkAdaptation::transportBlockSize(ue->currentCqi(), resourceBlocks));
            const double metric = achievable / ue->ewmaThroughput();
            if (metric > bestMetric) {
                bestMetric = metric;
                best = ue;
            }
        }
        return best ? SchedulingDecision{true, best->id()} : SchedulingDecision{};
    }

    std::string_view name() const noexcept override { return "ProportionalFair"; }
};

}  // namespace

SchedulerType parseSchedulerType(std::string_view name) {
    if (name == "rr" || name == "round-robin" || name == "roundrobin") {
        return SchedulerType::RoundRobin;
    }
    if (name == "maxci" || name == "max-ci" || name == "maxc/i") {
        return SchedulerType::MaxCI;
    }
    if (name == "pf" || name == "proportional-fair" || name == "proportionalfair") {
        return SchedulerType::ProportionalFair;
    }
    throw std::invalid_argument("unknown scheduler: " + std::string(name));
}

std::unique_ptr<Scheduler> makeScheduler(SchedulerType type) {
    switch (type) {
        case SchedulerType::RoundRobin:
            return std::make_unique<RoundRobinScheduler>();
        case SchedulerType::MaxCI:
            return std::make_unique<MaxCiScheduler>();
        case SchedulerType::ProportionalFair:
            return std::make_unique<ProportionalFairScheduler>();
    }
    return std::make_unique<ProportionalFairScheduler>();
}

}  // namespace ran::mac
