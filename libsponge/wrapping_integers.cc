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
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
     // Step 1: Wrap the checkpoint in the 32-bit space
    WrappingInt32 wrapped_checkpoint = wrap(checkpoint, isn);

    // Step 2: Compute the difference in the 32-bit space
    int32_t diff = n.raw_value() - wrapped_checkpoint.raw_value();

    // Step 3: Calculate the potential absolute sequence numbers
    // checkpoint + diff and checkpoint + diff + 2^32
    uint64_t abs_seqno_1 = checkpoint + static_cast<int64_t>(diff);
    uint64_t abs_seqno_2 = abs_seqno_1 + (1ull << 32);

    // Step 4: Select the closest absolute sequence number
    uint64_t closest_abs_seqno = (std::abs(static_cast<int64_t>(abs_seqno_1) - static_cast<int64_t>(checkpoint)) <=
                                  std::abs(static_cast<int64_t>(abs_seqno_2) - static_cast<int64_t>(checkpoint)))
                                     ? abs_seqno_1
                                     : abs_seqno_2;

    return closest_abs_seqno;
}
