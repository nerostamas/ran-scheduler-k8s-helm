#include "ran/mac/UserEquipment.hpp"

#include <algorithm>
#include <cmath>

namespace ran::mac {

UserEquipment::UserEquipment(UeId id, Cqi meanCqi, int harqProcesses,
                             Bits bufferBits) noexcept
    : id_(id),
      meanCqi_(std::clamp<Cqi>(meanCqi, kMinCqi, kMaxCqi)),
      currentCqi_(meanCqi_),
      bufferBits_(bufferBits) {
    harqProcs_.reserve(static_cast<std::size_t>(std::max(1, harqProcesses)));
    for (int i = 0; i < std::max(1, harqProcesses); ++i) {
        harqProcs_.emplace_back();
    }
}

void UserEquipment::updateCqi(std::mt19937& rng) noexcept {
    // Gauss-Markov random walk around the UE's mean CQI, clamped to [0, 15].
    std::normal_distribution<double> step(0.0, 1.0);
    const double drift = 0.5 * (static_cast<double>(meanCqi_) - currentCqi_);
    double next = static_cast<double>(currentCqi_) + drift + step(rng);
    next = std::clamp(next, static_cast<double>(kMinCqi),
                      static_cast<double>(kMaxCqi));
    currentCqi_ = static_cast<Cqi>(std::lround(next));
}

Bits UserEquipment::dequeueData(Bits bits) noexcept {
    const Bits taken = std::min(bits, bufferBits_);
    bufferBits_ -= taken;
    return taken;
}

HarqProcess* UserEquipment::findFreeHarq() noexcept {
    for (auto& h : harqProcs_) {
        if (!h.busy()) {
            return &h;
        }
    }
    return nullptr;
}

double UserEquipment::averageThroughput(Tti ttiCount) const noexcept {
    if (ttiCount == 0) {
        return 0.0;
    }
    const double seconds = static_cast<double>(ttiCount) * kTtiSeconds;
    return static_cast<double>(deliveredBits_) / seconds;
}

void UserEquipment::updateEwmaThroughput(double instantBitsPerTti,
                                         double alpha) noexcept {
    ewmaThroughput_ = (1.0 - alpha) * ewmaThroughput_ + alpha * instantBitsPerTti;
    if (ewmaThroughput_ < 1.0) {
        ewmaThroughput_ = 1.0;  // keep strictly positive for PF division
    }
}

}  // namespace ran::mac
