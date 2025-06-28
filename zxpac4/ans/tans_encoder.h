/**
 * @file tans_encoder.h
 * @author Jouni 'Mr.Spiv' korhonen
 * @version 0.1
 * @copyright The Unlicense
 *
 *
 *
 *
 */

#ifndef _TANS_H_INCLUDED
#define _TANS_H_INCLUDED

#include <cstdint>
#include "ans.h"

/*
 *
 *
 */
template<class T> class tans_encoder : public ans_base {
    T (*Ls_)[];
    T (*next_state_)[];
    T (*symbol_last_)[];
    uint8_t (*k_)[];
    int Ls_len_;
    ans_state_t state_;

    bool scaleSymbolFreqs(void);
    void buildEncodingTables(void);

public:
    tans_encoder(int m, const T* Ls, int n, debug_t debug=TRACE_LEVEL_NONE);
    virtual ~tans_encoder(void);
    const T* get_scaled_Ls(void) const;
    int get_Ls_len(void) const;
    void init_encoder(void);
    ans_state_t done_encoder(void) const;
    ans_state_t encode(T s, uint8_t& k, uint32_t& b);
};

#endif      // _TANS_H_INCLUDED
