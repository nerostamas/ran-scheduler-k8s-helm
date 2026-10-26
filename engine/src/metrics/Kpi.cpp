#include "ran/metrics/Kpi.hpp"

namespace ran::metrics {

double sum(const std::vector<double>& values) noexcept {
    double total = 0.0;
    for (const double v : values) {
        total += v;
    }
    return total;
}

double jainsFairnessIndex(const std::vector<double>& throughputs) noexcept {
    if (throughputs.empty()) {
        return 0.0;
    }
    double sumX = 0.0;
    double sumX2 = 0.0;
    for (const double x : throughputs) {
        sumX += x;
        sumX2 += x * x;
    }
    if (sumX2 == 0.0) {
        return 0.0;
    }
    const double n = static_cast<double>(throughputs.size());
    return (sumX * sumX) / (n * sumX2);
}

}  // namespace ran::metrics
