#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ran/Types.hpp"

namespace ran::mac {

class UserEquipment;

/// Result of a scheduling decision for one TTI: which UE wins the grant.
struct SchedulingDecision {
    bool granted = false;  ///< false means no eligible UE this TTI
    UeId ueId = 0;         ///< winner when granted == true
};

/// Abstract base for MAC-layer (L2) downlink schedulers.
///
/// A scheduler decides, every TTI, which UE receives the shared resource-block
/// grant. Implementations differ only in the metric they maximize.
class Scheduler {
public:
    virtual ~Scheduler() = default;

    /// Choose a UE to serve this TTI from the `candidates` that have data to send.
    /// @param candidates  UEs with a non-empty buffer and a free HARQ process.
    /// @param resourceBlocks  RBs available in the grid this TTI.
    [[nodiscard]] virtual SchedulingDecision schedule(
        const std::vector<UserEquipment*>& candidates, int resourceBlocks) const = 0;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
};

/// Scheduling strategy selector.
enum class SchedulerType { RoundRobin, MaxCI, ProportionalFair };

/// Parse a scheduler name ("rr", "maxci", "pf"). Throws std::invalid_argument
/// on an unknown value.
SchedulerType parseSchedulerType(std::string_view name);

/// Factory for concrete schedulers.
std::unique_ptr<Scheduler> makeScheduler(SchedulerType type);

}  // namespace ran::mac
