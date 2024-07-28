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
    WrappingInt32 chk = wrap(checkpoint, isn);
    uint32_t num_wraps = checkpoint / MAX_INT32;
    if(n.raw_value() > chk.raw_value()){
        uint32_t diff = n - chk;
        uint64_t candidate_r = checkpoint + diff;
        uint64_t candidate_l = candidate_r - MAX_INT32;
        return MAX_INT32 - diff < diff ? candidate_l : candidate_r;
    }else{
        uint32_t diff = chk - n;
        uint64_t candidate_l = checkpoint - diff;
        uint64_t candidate_r = candidate_l + MAX_INT32;
        return MAX_INT32 - diff < diff ? candidate_r : candidate_l;
    }
}

