/**
 * @file inc/rice_encoder.h
 * @version v0.1 20251130
 * @author Jouni 'Mr.Spiv' Korhonen
 * @brief A RICE encoder template class implementation
 * @copyright The Unlicense
 *
 * Good explanations of Rice (Unary) & Rice-Golomb encoding can be found at:
 *   https://en.wikipedia.org/wiki/Unary_coding
 *   https://en.wikipedia.org/wiki/Golomb_coding
 *   https://michaeldipperstein.github.io/rice.html
 *
 */

#ifndef _RICE_ENCODER_H_INCLUDED
#define _RICE_ENCODER_H_INCLUDED

#include <cstdint>
#include <cassert>


// Rice-Glolomb encoder
//
template<int M, int B>
class rg_encoder {
public:
    rg_encoder(void);
    ~rg_encoder(void) {}
    int encode_value(uint32_t& value);
};
  
// Plain Rice encoder
//
template<int K>
class rice_encoder { 
public:
    rice_encoder(void) {}
    ~rice_encoder(void) {}
    int encode_value(uint32_t& value);
};

// Implementations..
//
//

template<int M, int B>
rg_encoder<M,B>::rg_encoder(void) 
{
    // M has to be greater than pow2(floor(log2(M)))
    assert(M > (1 << B));
    // M has to be smaller than pow2(floor(log2(M))+1)
    assert(M < (1 << (B + 1)));
    // M cannot be a power of two
    assert(M & (M - 1));
}

template<int M, int B>
int rg_encoder<M,B>::encode_value(uint32_t& value)
{
    uint32_t r,q;
    int n = 1;

    q = value / M;          // q = floor(value / M)
    r = value - q * M;      // r = value mod M)
    value = 0;

    // Quontient as a run of 1-bits
    while (q > 0) {
        value = (value + 1) << 1;
        --q; ++n;
    }

    // Check if we can encode the reminder in B or B+1 bits
    if (r < ((1 << (B + 1)) - M)) {
        value = (value << B) | r;
        n = n + B;
    } else {
        r = r + (1 << (B + 1)) - M;
        value = (value << (B + 1)) | r;
        n = n + B + 1;
    }

    return n;
}

// Implementations..
//
//

template<int K>
int rice_encoder<K>::encode_value(uint32_t& value)
{
    uint32_t r,q;
    int n = 1;

    r = value & ((1 << K) - 1);
    q = value >> K;
    value = 0;

    // Quontient as a run of 1-bits i.e. a unary code
    while (q > 0) {
        value = (value + 1) << 1;
        --q; ++n;
    }

    // Add erminder
    value = (value << K) | r;
    return n + K;
}

#endif // _RICE_ENCODER_H_INCLUDED

