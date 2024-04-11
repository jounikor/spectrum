/**
 * @file hash.h
 * @version 0.1
 * @date 10-Apr-2024
 * @author Jouni 'Mr.Spiv' Korhonen
 * @brief Hash-based string matcher for LZ engines.
 * @copyright The Unlicense
 */

#ifndef _HASH_H_INCLUDED
#define _HASH_H_INCLUDED

#include <iostream>
#include "lz_util.h"

// https://www3.risc.jku.at/people/hemmecke/AldorCombinat/combinatse20.html
// No magic here. I just selected one prime that fits between 15 and 16 bits.
#define HASH3_MAGIC 46027

#define HASH3_SIZE  (1<<13)
#define HASH3_MASK  (HASH3_SIZE-1)
#define HASH3_SHFT  16

class hash3_match {
    int* m_head;
    int* m_next;
    match* m_mtch;
    int m_mask;
    int m_max_chain;
    int m_min_match;
    int m_max_match;

    int hash(const char *buf, int pos, int len)
    {
        unsigned int hh = 0;

        switch (len-pos-3) {
        case -1:
            hh = static_cast<uint8_t>(buf[pos++]);
        case -2:
            hh ^= (static_cast<uint8_t>(buf[pos++]) << 4);
        default:
            hh ^= (static_cast<uint8_t>(buf[pos]) << 8);
        }

        return ((hh * HASH3_MAGIC) >> HASH3_SHFT) & HASH3_MASK; 
    }
public:
    hash3_match(int window_size, int min_match, int max_match);
    ~hash3_match(void);
    
    void init_get_matches(int max_matches, match *matches = NULL);
    int find_matches(const char *buf, int pos, int len, bool only_better_matches);
    void reinit(void);
};

#endif
