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

/*
 *
 */
typedef uint32_t ans_state_t;

/*
 *
 *
 */
class ans_base {
protected:
    int SPREAD_STEP_;
    int INITIAL_STATE_;
    int M_;
    int M_MASK_;

public:
    uint8_t get_k_(int base, int threshold, uint8_t min_k=1) {
        // The minimum k setting is for tANS, where we always send
        // at least 1 bit..
        uint8_t k = min_k;
        while ((base << k) < threshold) { ++k; }
        return k;
    }

    int spreadFunc_(int x) {
        return (x + SPREAD_STEP_) & M_MASK_;
    }

public:
    ans_base(int m) {

        if (m & (m - 1)) {
            std::stringstream ss;
            ss <<__FILE__ << ":" << __LINE__ << " -> M is not a power of two.";
            throw std::invalid_argument(ss.str());
        }
        M_ = m;
        M_MASK_ = m - 1;
    
        // Initialize static variables
        SPREAD_STEP_ = 5;
        INITIAL_STATE_ = 3;
    }
    virtual ~ans_base(void) {
    }
};

#endif      // _ANS_H_INCLUDED
