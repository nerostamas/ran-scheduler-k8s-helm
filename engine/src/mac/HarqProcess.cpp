#include "ran/mac/HarqProcess.hpp"

namespace ran::mac {

void HarqProcess::startNewTransmission(Bits bits) noexcept {
    busy_ = true;
    attempt_ = 1;
    pendingBits_ = bits;
}

void HarqProcess::onAck() noexcept {
    busy_ = false;
    attempt_ = 0;
    pendingBits_ = 0;
}

bool HarqProcess::onNack() noexcept {
    if (!busy_) {
        return false;
    }
    if (attempt_ >= maxTransmissions_) {
        // Exhausted all attempts: drop the block and free the process.
        busy_ = false;
        attempt_ = 0;
        pendingBits_ = 0;
        return false;
    }
    ++attempt_;
    return true;
}

}  // namespace ran::mac
