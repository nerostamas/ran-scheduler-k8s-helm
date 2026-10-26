#include "ran/phy/LinkAdaptation.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace ran::phy {
namespace {

/// Spectral-efficiency target (bits/symbol) per CQI index, loosely following
/// 3GPP TS 36.213 Table 7.2.3-1. Monotonically non-decreasing in CQI.
constexpr std::array<double, 16> kSpectralEfficiency = {
    0.0000,  // CQI 0  : out of range / no transmission
    0.1523,  // CQI 1  : QPSK
    0.2344, 0.3770, 0.6016, 0.8770, 1.1758,  // QPSK
    1.4766, 1.9141, 2.4063,                  // 16QAM
    2.7305, 3.3223, 3.9023, 4.5234, 5.1152, 5.5547  // 64QAM
};

}  // namespace

Mcs LinkAdaptation::mcsForCqi(Cqi cqi) noexcept {
    const Cqi c = std::min(cqi, kMaxCqi);
    if (c <= 6) {
        return Mcs{2, kSpectralEfficiency[c] / 2.0};  // QPSK
    }
    if (c <= 9) {
        return Mcs{4, kSpectralEfficiency[c] / 4.0};  // 16QAM
    }
    return Mcs{6, kSpectralEfficiency[c] / 6.0};  // 64QAM
}

Bits LinkAdaptation::transportBlockSize(Cqi cqi, int resourceBlocks) noexcept {
    const Cqi c = std::min(cqi, kMaxCqi);
    if (c == 0 || resourceBlocks <= 0) {
        return 0;
    }
    const double bitsPerSymbol = kSpectralEfficiency[c];
    const double symbols =
        static_cast<double>(resourceBlocks) * kSymbolsPerRbPerTti;
    return static_cast<Bits>(std::floor(bitsPerSymbol * symbols));
}

}  // namespace ran::phy
