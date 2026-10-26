#pragma once

#include <random>
#include <vector>

#include "ran/Types.hpp"
#include "ran/mac/HarqProcess.hpp"

namespace ran::mac {

/// Models one connected UE from the gNB/eNB's perspective.
///
/// Holds per-UE state that the scheduler needs: channel quality (CQI) that
/// changes every TTI, a downlink buffer of pending data, HARQ processes, and
/// throughput accumulators.
class UserEquipment {
public:
    /// @param id   UE identifier.
    /// @param meanCqi  Mean CQI for the random walk channel model.
    /// @param harqProcesses  Number of stop-and-wait HARQ processes.
    /// @param bufferBits  Initial downlink buffer size in bits.
    UserEquipment(UeId id, Cqi meanCqi, int harqProcesses = 8,
                  Bits bufferBits = 10'000'000) noexcept;

    [[nodiscard]] UeId id() const noexcept { return id_; }

    // ---- Channel feedback ----

    /// Report a fresh CQI value for the current TTI.
    void updateCqi(std::mt19937& rng) noexcept;

    [[nodiscard]] Cqi currentCqi() const noexcept { return currentCqi_; }

    // ---- Downlink buffer ----

    [[nodiscard]] Bits bufferSize() const noexcept { return bufferBits_; }

    void enqueueData(Bits bits) noexcept { bufferBits_ += bits; }

    /// Dequeue up to `bits` from the buffer. Returns the actual dequeued amount.
    Bits dequeueData(Bits bits) noexcept;

    // ---- HARQ ----

    /// Get a free HARQ process, or nullptr if all are busy.
    HarqProcess* findFreeHarq() noexcept;

    /// Access all HARQ processes (e.g., to drive feedback).
    [[nodiscard]] std::vector<HarqProcess>& harqProcesses() noexcept { return harqProcs_; }

    // ---- Throughput accounting ----

    void recordDelivered(Bits bits) noexcept { deliveredBits_ += bits; }

    [[nodiscard]] Bits totalDelivered() const noexcept { return deliveredBits_; }

    /// Average throughput (bps) over a window of `ttiCount` TTIs.
    [[nodiscard]] double averageThroughput(Tti ttiCount) const noexcept;

    /// Exponentially weighted moving average throughput used by PF scheduler.
    [[nodiscard]] double ewmaThroughput() const noexcept { return ewmaThroughput_; }

    void updateEwmaThroughput(double instantBitsPerTti, double alpha = 0.001) noexcept;

private:
    UeId id_;
    Cqi meanCqi_;
    Cqi currentCqi_;
    Bits bufferBits_;
    Bits deliveredBits_ = 0;
    double ewmaThroughput_ = 1.0;  // start > 0 to avoid div-by-zero
    std::vector<HarqProcess> harqProcs_;
};

}  // namespace ran::mac
