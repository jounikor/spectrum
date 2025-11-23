/**
 * @file hash.cpp
 * @version 0.2
 * @author Jouni 'Mr.Spiv' Korhonen
 *
 *
 * @copyright The Unlicense
 */

#include <iostream>
#include <cassert>
#include "lz_util.h"
#include "hash.h"

// Handle accordingly.. depending on your implementation
//#ifdef WINDOW_IS_POWER_OF_TWO
//#undef WINDOW_IS_POWER_OF_TWO
//#endif


hash3::hash3(int window_size, int min_match, int max_match,
    int good_match,
    int mm2_thres_offset,
    int mm3_thres_offset):
    m_head(NULL),
    m_next(NULL),
    m_mtch(NULL)
{
#ifdef WINDOW_IS_POWER_OF_TWO
    if ((window_size-1) & window_size) {
        EXCEPTION(std::invalid_argument,"Window size not a power of two.");
    }

    m_mask = window_size-1;
#else
    m_mask = window_size;
#endif // WINDOW_IS_POWER_OF_TWO
    m_head = new int[HASH_SIZE];
    m_next = new int[window_size];
    m_min_match = min_match;
    m_max_match = max_match;
    m_good_match = good_match;
    m_min_match2_threshold_offset = mm2_thres_offset;
    m_min_match3_threshold_offset = mm3_thres_offset;
    reinit();
}

void hash3::impl_reinit(void)
{
    int n;

    for (n = 0; n < HASH_SIZE; n++) {
        m_head[n] = -1;
    }
}


hash3::~hash3(void)
{
    if (m_head) {
        delete[] m_head;
    }
    if (m_next) {
        delete[] m_next;
    }
}

void hash3::impl_init_get_matches(int max_matches, match *matches)
{
    m_mtch = matches;
    m_max_chain = max_matches;
}

/**
 *
 *
 *
 * @return Number of found matches or 0.
 */

int hash3::impl_find_matches(const char *buf, int pos, int len, bool only_better_matches=false)
{
    int length;
    int best   = m_min_match - 1;
#ifdef WINDOW_IS_POWER_OF_TWO
    int low    = pos - m_mask;
#else
    int low    = pos - m_mask;
#endif  // WINDOW_IS_POWER_OF_TWO
    int found  = 0;
    int head   = hash(buf,pos);
    int next   = m_head[head];
    const char* m;
    const char* n;
    const char* e;


    if (low < 0) {
        low = 0;
    }
    if (len > m_max_match) {
        len = m_max_match;
    }

    e = buf + pos + len;

    while (next >= low && found < m_max_chain) {
        m = buf+next;
        n = buf+pos;
        length = 0;

        if (only_better_matches) {
            if (m[best] != n[best]) {
                goto next_in_chain;
            }
        }

        while (*m++ == *n++ && ++length < len && n < e);
        assert(length <= m_max_match);

        if (length >= m_min_match) {
			if (only_better_matches) {
                if (length > best) {
                    best = length;
                } else {
                    goto next_in_chain;
                }
            }
            m_mtch[found].length = length;
            m_mtch[found].offset = pos-next;
            ++found;

            if (length >= m_good_match || length >= m_max_match - 1) {
                break;
            }
        }
next_in_chain:
        // Advance to the next history position in the hash chain
#ifdef WINDOW_IS_POWER_OF_TWO
        next = m_next[next & m_mask];
#else
        next = m_next[next % m_mask];
#endif // WINDOW_IS_POWER_OF_TWO
    }
    // Update the hash chain with the current position..
#ifdef WINDOW_IS_POWER_OF_TWO
    m_next[pos & m_mask] = m_head[head];
#else
    m_next[pos % m_mask] = m_head[head];
#endif // WINDOW_IS_POWER_OF_TWO
    m_head[head] = pos;

    return found;
}

