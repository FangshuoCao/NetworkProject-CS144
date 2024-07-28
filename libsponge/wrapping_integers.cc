#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    //casting uint64 to uint32 is the same as % 2^32
    return WrappingInt32{static_cast<uint32_t>(n) + isn.raw_value()};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
#include <cstdint>
#include "wrapping_integers.hh"

uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    const uint64_t MAX_INT32 = 1ull << 32;

    // Step 1: Calculate the offset in the 32-bit space
    uint32_t offset = n - isn;

    // Step 2: Calculate potential absolute sequence numbers
    uint64_t candidate1 = ((checkpoint / MAX_INT32) * MAX_INT32) + offset;
    uint64_t candidate2 = candidate1 + MAX_INT32;

    // Step 3: Select the candidate closest to the checkpoint
    if (std::abs(static_cast<int64_t>(candidate1) - static_cast<int64_t>(checkpoint)) <=
        std::abs(static_cast<int64_t>(candidate2) - static_cast<int64_t>(checkpoint))) {
        return candidate1;
    } else {
        return candidate2;
    }
}

uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    const uint64_t MAX_INT32 = 1ull << 32;
    uint64_t offset = n - isn;
    uint32_t num_wraps = checkpoint / MAX_INT32;
    uint64_t chk = (checkpoint - isn.raw_value()) % MAX_INT32;
    if(num_wraps == 0){
        return offset;
    }else{
        if(offset > chk){
            uint64_t absr = MAX_INT32 * num_wraps + offset;
            uint64_t absl = absr - MAX_INT32;
            return absr - checkpoint < checkpoint - absl ? absr : absl;
        }else{
            uint64_t absl = MAX_INT32 * num_wraps + offset;
            uint64_t absr = absl + MAX_INT32;
            return absr - checkpoint < checkpoint - absl ? absr : absl;
        }
    }
}

