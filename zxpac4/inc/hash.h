/**
 * @file hash.h
 * @version 0.1
 * @date 10-Apr-2024
 * @author Jouni 'Mr.Spiv' Korhonen
 * @brief Hash-based string matcher for LZ engines.
 * @copyright The Unlicense
 *
 */

#ifndef _HASH_H_INCLUDED
#define _HASH_H_INCLUDED

#include <iostream>
#include "lz_util.h"
#include <cstdint>

// https://www3.risc.jku.at/people/hemmecke/AldorCombinat/combinatse20.html
// No magic here. I just selected one prime that fits between 15 and 16 bits.
#define HASH_MAGIC 46027

#define HASH_SIZE  (1<<16)
#define HASH_MASK  (HASH_SIZE-1)


class hash3 : public lz_match {
    int* m_head;
    int* m_next;
    match* m_mtch;
    int m_mask;
    int m_max_chain;
    int m_min_match;
    int m_max_match;
    int m_good_match;
    int m_min_match2_threshold_offset;
    int m_min_match3_threshold_offset;

    int hash(const char *buf, int pos)
    {
        // Note, this function does not care about the end of file.
        // Having enough buffer beyond the end of the file is left
        // to the caller.
        unsigned int hh = 0;
        hh = (static_cast<uint8_t>(buf[pos++]) << 8);
        hh |= static_cast<uint8_t>(buf[pos]);
        return hh; 
    }

public:
    hash3(int window_size, int min_match, int max_match,
        int good_match,
        int mm2_thres_offset,
        int mm3_thres_offset);
    ~hash3(void);
    
    void init_get_matches(int max_matches, match *matches = NULL);
    int find_matches(const char *buf, int pos, int len, bool only_better_matches);
    void reinit(void);
};
#endif
