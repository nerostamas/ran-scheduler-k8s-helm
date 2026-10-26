#pragma once

#include "ran/Types.hpp"

namespace ran::mac {

/// A single stop-and-wait HARQ process (L2 reliability).
///
/// Models the essential HARQ behaviour: a transmission is either ACKed (success)
/// or NACKed (needs retransmission). Each retransmission accumulates "soft"
/// combining gain, modeled here as an increasing success probability, up to a
/// maximum number of attempts before the block is dropped.
class HarqProcess {
public:
    explicit HarqProcess(int maxTransmissions = 4) noexcept
        : maxTransmissions_(maxTransmissions) {}

    /// True when the process holds an outstanding (not-yet-ACKed) transport block.
    [[nodiscard]] bool busy() const noexcept { return busy_; }

    /// Current transmission attempt for the outstanding block (1-based), or 0 if idle.
    [[nodiscard]] int attempt() const noexcept { return attempt_; }

    [[nodiscard]] int maxTransmissions() const noexcept { return maxTransmissions_; }

    [[nodiscard]] Bits pendingBits() const noexcept { return pendingBits_; }

    /// Start a new HARQ transmission. Precondition: !busy().
    void startNewTransmission(Bits bits) noexcept;

    /// Register a positive acknowledgement; frees the process.
    void onAck() noexcept;

    /// Register a negative acknowledgement.
    /// @return true if a retransmission is still allowed, false if the block is
    ///         dropped after exhausting `maxTransmissions` (process freed).
    bool onNack() noexcept;

private:
    int maxTransmissions_;
    int attempt_ = 0;
    bool busy_ = false;
    Bits pendingBits_ = 0;
};

}  // namespace ran::mac
