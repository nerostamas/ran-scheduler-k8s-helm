#pragma once

#include <vector>

namespace ran::metrics {

/// Jain's fairness index for a set of per-UE throughput values.
/// Returns a value in (0, 1]; 1.0 means perfectly equal allocation.
/// An empty input or all-zero input returns 0.0.
double jainsFairnessIndex(const std::vector<double>& throughputs) noexcept;

/// Sum of per-UE throughput values (e.g., total cell throughput).
double sum(const std::vector<double>& values) noexcept;

}  // namespace ran::metrics
