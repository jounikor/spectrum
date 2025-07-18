/**
 * @file tans_decoder_k.h
 * @author Jouni 'Mr.Spiv' korhonen
 * @version 0.1
 * @copyright The Unlicense
 * @brief Table ANS decoder template class implementation with k-tables.
 *
 *
 *
 */

#ifndef _TANS_DECODER_K_H_INCLUDED
#define _TANS_DECODER_K_H_INCLUDED

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
    uint8_t k_[M];

public:
    tans_decoder(const T* Ls, int n, debug_e debug=TRACE_LEVEL_NONE);
    ~tans_decoder();
    void init_decoder(ans_state_t& state);
    void done_decoder(ans_state_t& state) const;
    T decode(ans_state_t& state, uint8_t& k);
    void next_state(ans_state_t& state, uint32_t b) const;
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

        // The following loop is not executed is the symbol frequency is 0.
        for (int p = c; p < 2*c; p++) {
            // Note that we make the table indices reside between [0..L),
            // since values within [L..2L) % M is within [0..M). This is 
            // a simple algorithmic optimization to avoid extra index
            // adjusting during decoding..

            y_[xp] = p;
            L_[xp] = s;
            k_[xp] = get_k_(p,M);

            TRACE_DBUG(s << ", " << c << ", " << p << ", "
                << static_cast<uint32_t>(k_[xp]) << ", "
                << xp << std::endl)
            

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
void tans_decoder<T,M>::init_decoder(ans_state_t& state)
{
    // This is essentially a dummy function
}

template<class T, int M>
void tans_decoder<T,M>::done_decoder(ans_state_t& state) const
{
    // This is essentially a dummy function
}

/**
 *
 * @returns Intermediate state, which is k-bits shifted to the left but
 *          has lower k-bits not still not read from the input.
 */

template<class T, int M>
T tans_decoder<T,M>::decode(ans_state_t& state, uint8_t& k)
{
    TRACE_DBUG("Old state: 0x" << std::hex << state << std::dec << std::endl)

    T s = L_[state];
    k = k_[state];
    state = y_[state] << k;
    return s;
}

template<class T, int M> 
void tans_decoder<T,M>::next_state(ans_state_t& state, uint32_t b) const
{
    state = (state + b) & (M-1);
}

#endif      // _TANS_DECODER_K_H_INCLUDED
