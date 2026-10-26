#pragma once

#include <cstdint>

namespace ran {

/// Identifier for a User Equipment (UE).
using UeId = std::uint32_t;

/// A Transmission Time Interval index (1 ms in LTE numerology 0).
using Tti = std::uint64_t;

/// Channel Quality Indicator, 0..15 (3GPP TS 36.213 Table 7.2.3-1 style).
using Cqi = std::uint8_t;

/// Number of bits.
using Bits = std::uint64_t;

inline constexpr Cqi kMinCqi = 0;
inline constexpr Cqi kMaxCqi = 15;

/// TTI duration in seconds for LTE numerology 0 (1 ms).
inline constexpr double kTtiSeconds = 1e-3;

}  // namespace ran
