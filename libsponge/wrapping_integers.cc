#include "wrapping_integers.hh"
#include <cstdlib>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

#define TWOTO32 4294967296

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t mod_result = static_cast<uint32_t>(((n%TWOTO32)+(static_cast<uint64_t>(isn.raw_value())%TWOTO32))%TWOTO32);
    return WrappingInt32{mod_result};
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
    uint64_t distance;

    auto uint64AbsDiff = [](uint64_t val1, uint64_t val2){
        return (val1>=val2) ? val1-val2 : val2-val1;
    };

    if(isn.raw_value()<=n.raw_value()){
        distance = n.raw_value()-isn.raw_value();
    }
    else{
        distance = TWOTO32-isn.raw_value()+n.raw_value();
    }

    uint64_t mul = checkpoint/TWOTO32;
    uint64_t first_round_result;

    if(uint64AbsDiff(checkpoint, (TWOTO32*mul+distance))>=uint64AbsDiff(TWOTO32*(mul+1)+distance, checkpoint)){
        first_round_result = TWOTO32*(mul+1)+distance;
    }
    else{
        first_round_result = TWOTO32*mul+distance;
    }

    if(uint64AbsDiff(checkpoint, first_round_result)>=uint64AbsDiff(checkpoint, TWOTO32*(mul-1)+distance)){
        return TWOTO32*(mul-1)+distance;
    }
    else{
        return first_round_result;
    }
}
