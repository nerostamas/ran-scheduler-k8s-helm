#pragma once

#include <memory>
#include <random>
#include <vector>

#include "ran/Types.hpp"
#include "ran/mac/Scheduler.hpp"
#include "ran/mac/UserEquipment.hpp"

namespace ran {

/// Configuration for a simulation run.
struct SimConfig {
    int numUes = 10;
    int resourceBlocks = 50;   ///< RBs in the grid (50 RBs ~ 10 MHz LTE)
    Tti numTtis = 1000;
    unsigned seed = 42;
    mac::SchedulerType scheduler = mac::SchedulerType::ProportionalFair;
};

/// Aggregated results after a run.
struct SimResult {
    double cellThroughputMbps = 0.0;
    double jainsFairness = 0.0;
    double harqRetxRate = 0.0;  ///< fraction of transmissions that were retransmissions
    std::vector<double> perUeThroughputMbps;
};

/// Drives the TTI loop: updates channels, picks a UE via the scheduler, performs
/// L1 link adaptation to size the transport block, applies HARQ feedback, and
/// accumulates KPIs. This is the glue that ties L1 and L2 together.
class Simulator {
public:
    explicit Simulator(SimConfig config);

    /// Run the full simulation and return aggregated KPIs.
    SimResult run();

    [[nodiscard]] const SimConfig& config() const noexcept { return config_; }

private:
    SimConfig config_;
    std::mt19937 rng_;
    std::vector<mac::UserEquipment> ues_;
    std::unique_ptr<mac::Scheduler> scheduler_;
};

}  // namespace ran
