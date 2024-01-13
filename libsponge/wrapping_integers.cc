#include "wrapping_integers.hh"

#include <iostream>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // DUMMY_CODE(n, isn);

    return WrappingInt32{static_cast<uint32_t>((isn.raw_value() + n) % (1ul << 32))};
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
    // DUMMY_CODE(n, isn, checkpoint);
    // uint32_t diff = static_cast<uint32_t>(n - isn);

    // uint64_t base = checkpoint >> 32 << 32;  // Closest base to checkpoint
    // uint64_t candidate = base + diff;        // Candidate for closest absolute number

    // if (candidate >= checkpoint) {
    //     if (candidate - checkpoint > (1ul << 31)) {
    //         candidate -= (1ul << 32);
    //     }
    // } else {
    //     if (checkpoint - candidate > (1ul << 31)) {
    //         candidate += (1ul << 32);
    //     }
    // }

    // return candidate;
    // the implementation above is wrong, because what we need is the closest number to checkpoint, not the closest number to base!!!!!!!!!!!!!!!!!
    DUMMY_CODE(n, isn, checkpoint);
    uint32_t diff = static_cast<uint32_t>(n - isn);

    uint64_t base1 = (checkpoint - (1 << 31)) & 0xFFFFFFFF00000000;
    uint64_t base2 = (checkpoint + (1 << 31)) & 0xFFFFFFFF00000000;

    uint64_t res1 = base1 | diff, res2 = base2 | diff;
    return (max(res1, checkpoint) - min(res1, checkpoint) < max(res2, checkpoint) - min(res2, checkpoint)) ? res1 : res2;
}
