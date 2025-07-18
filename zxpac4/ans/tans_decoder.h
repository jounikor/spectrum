/**
 * @file tans_decoder.h
 * @author Jouni 'Mr.Spiv' korhonen
 * @version 0.1
 * @copyright The Unlicense
 * @brief Table ANS decoder template class implementation.
 *
 *
 *
 */

#ifndef _TANS_DECODER_H_INCLUDED
#define _TANS_DECODER_H_INCLUDED

#include <cstdint>
#include "ans.h"

/*
 *
 *
 */
template<class T, int M>
class tans_decoder : public ans_base {
    T L_[M];
    T y_[M];

public:
    tans_decoder(const T* Ls, int n, debug_e debug=TRACE_LEVEL_NONE);
    ~tans_decoder();
    ans_state_t init_decoder(ans_state_t initial_state);
    ans_state_t done_decoder(ans_state_t state) const;
    T decode(ans_state_t& state, uint8_t& k);
    ans_state_t next_state(ans_state_t intermediate_state, uint32_t b) const;
};

template<class T, int M>
tans_decoder<T,M>::~tans_decoder()
{
}

template<class T, int M>
tans_decoder<T,M>::tans_decoder(const T* Ls, int Ls_len, debug_e debug) :
    ans_base(M,debug)
{
    // The initial state to start with..
    int xp = INITIAL_STATE_;

    for (int s = 0; s < Ls_len; s++) {
        int c = Ls[s];

        for (int p = c; p < 2*c; p++) {
            // Note that we make the table indices reside between [0..L),
            // since values within [L..2L) % M is within [0..M). This is 
            // a simple algorithmic optimization to avoid extra index
            // adjusting during decoding..
            // This optimization does not come for free and introduces a 
            // corner case when state==0. During decoding and next state
            // calculation that has to be taken into account if k-values
            // are not taken from a precalculated table.

            TRACE_DBUG(s << ", " << c << ", " << p << ", " << xp << std::endl)

            y_[xp] = p;
            L_[xp] = s;

            // advance to the next state..
            xp = spreadFunc_(xp);
        }
    }
    TRACE_DBUG("L_ and Y_ tables:" << std::endl)
    for (int i = 0; i < M; i++) {
        TRACE_DBUG(i << ":\t" << static_cast<uint32_t>(L_[i]) << ",\t"
            << static_cast<uint32_t>(y_[i])
            << std::endl)
    }
}

template<class T, int M>
ans_state_t tans_decoder<T,M>::init_decoder(ans_state_t initial_state)
{
    // This is essentially a dummy function
    return initial_state;
}

template<class T, int M>
ans_state_t tans_decoder<T,M>::done_decoder(ans_state_t state) const
{
    return state;
}

/**
 *
 * @brief k-tableless symbol decoder.
 *
 *   It is possible (and simple) to lookup k-value from a table. However,
 *   That would require yet another M bytes size table and with certain
 *   8-bit systems that is not desired.
 *
 * @returns Intermediate state, which is k-bits shifted to the left but
 *          has lower k-bits not still not read from the input.
 */

template<class T, int M>
T tans_decoder<T,M>::decode(ans_state_t& state, uint8_t& k)
{
    TRACE_DBUG("Old state: 0x" << std::hex << state << std::dec << std::endl)

    T s = L_[state];
    k = 0;
    state = y_[state];
    assert(state > 0);

    do {
        // This is a bit akward/slow solution, but I wanted to isolate
        // the bit input entirely away from the next state calculation. 
        // 
        // An alternative implementation is to input 'b' bit by bit each
        // time the old state is shifted left. That's how I would do it
        // using Z80, for example.

        state <<= 1;   // k-tableless optimization
        k++;
    } while (state < M);

    return s;
}

template<class T, int M> 
ans_state_t tans_decoder<T,M>::next_state(ans_state_t intermediate_state, uint32_t b) const
{
    return (intermediate_state + b) & (M-1);
}

#endif      // _TANS_DECODER_H_INCLUDED
