/**
 * @file ans.h
 * @author Jouni 'Mr.Spiv' korhonen
 * @version 0.1
 * @copyright The Unlicense
 *
 *
 *
 *
 */

#ifndef _ANS_H_INCLUDED
#define _ANS_H_INCLUDED

#include <cstdint>
#include <stdexcept>
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream

#include "trace.h"

/*
 *
 *
 */
class ans_base {
    static int SPREAD_STEP_;
    static int INITIAL_STATE_;
protected:
    int M_;
    int M_MASK_;
    debug_t DEBUG_;

public:
    int get_k_(int base, int threshold, int min_k=1) {
        // The minimum k setting is for tANS, where we always send
        // at least 1 bit..
        int k = min_k;
        while ((base << k) < threshold) { ++k; }
        return k;
    }

    int spreadFunc_(int x) {
        return (x + SPREAD_STEP_) & M_MASK_;
    }

public:
    int debug(debug_t value=TRACE_LEVEL_NONE) {
        int old = DEBUG_;
        DEBUG_ = value;
        return old;
    }

    ans_base(int m, debug_t debug=0) {
        DEBUG_ = debug;
        if (m & (m - 1)) {
            std::stringstream ss;
            ss <<__FILE__ << ":" << __LINE__ << " -> M is not a power of two.";
            throw std::invalid_argument(ss.str());
        }
        M_ = m;
        M_MASK_ = m - 1;
    }
    virtual ~ans_base(void) {
    }
};


// Initialize static variables
int ans_base::SPREAD_STEP_ = 5;
int ans_base::INITIAL_STATE_ = 3;




#endif      // _ANS_H_INCLUDED
