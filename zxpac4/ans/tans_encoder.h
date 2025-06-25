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
    T* Ls_;
    T* k_;
    T* next_state_;
    T* symbol_last_;
    int Ls_len_;

    bool scaleSymbolFreqs(void);
    void buildEncodingTables(void);

public:
    tans_encoder(int m, const T* Ls, int n, debug_t debug=TRACE_LEVEL_NONE);
    virtual ~tans_encoder(void);
};

#endif      // _TANS_H_INCLUDED
