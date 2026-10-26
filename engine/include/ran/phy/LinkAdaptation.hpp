#pragma once

#include "ran/Types.hpp"

namespace ran::phy {

/// Modulation and Coding Scheme derived from a CQI report.
struct Mcs {
    int modulationOrder;    ///< bits per symbol (2=QPSK, 4=16QAM, 6=64QAM, 8=256QAM)
    double codeRate;        ///< effective channel code rate in (0, 1]
};

/// L1 link adaptation: maps channel quality (CQI) to a modulation & coding
/// scheme (MCS) and to the transport block size (TBS) that fits on a given
/// number of resource blocks in one TTI.
///
/// This is a simplified, deterministic model of the 3GPP CQI/MCS/TBS tables.
/// It is monotonic in CQI, which is the property schedulers rely on.
class LinkAdaptation {
public:
    /// Modulation + coding scheme selected for a reported CQI.
    static Mcs mcsForCqi(Cqi cqi) noexcept;

    /// Transport block size (in bits) achievable on `resourceBlocks` RBs for
    /// the given CQI during one TTI. Returns 0 when the link cannot carry data.
    static Bits transportBlockSize(Cqi cqi, int resourceBlocks) noexcept;

    /// Number of modulation symbols carried by one RB in one TTI.
    /// (12 subcarriers x 14 OFDM symbols, minus a fixed overhead estimate.)
    static constexpr int kSymbolsPerRbPerTti = 12 * 14 - 36;
};

}  // namespace ran::phy
