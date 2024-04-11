/**
 * @file hash.cpp
 * @version 0.1
 * @author Jouni 'Mr.Spiv' Korhonen
 *
 *
 * @copyright The Unlicense
 */



#include <iostream>
#include "lz_util.h"
#include "hash.h"


hash3_match::hash3_match(int window_size, int min_match, int max_match):
    m_head(NULL),
    m_next(NULL),
    m_mtch(NULL)
{
    if ((window_size-1) & window_size) {
        EXCEPTION(std::invalid_argument,"Window size not a power of two.");
    }

    m_head = new int[HASH3_SIZE];
    m_next = new int[window_size];
    m_mask = window_size-1;
    m_min_match = min_match;
    m_max_match = max_match;
    reinit();
}

void hash3_match::reinit(void)
{
    int n;

    for (n = 0; n < HASH3_SIZE; n++) {
        m_head[n] = -1;
    }
    //for (n = 0; n < m_mask+1; n++) {
    //    m_next[n] = -1;
    //}
}


hash3_match::~hash3_match(void)
{
    if (m_head) {
        delete[] m_head;
    }
    if (m_next) {
        delete[] m_next;
    }
}

void hash3_match::init_get_matches(int max_matches, match *matches)
{
    m_mtch = matches;
    m_max_chain = max_matches;
}

int hash3_match::find_matches(const char *buf, int pos, int len, bool only_better_matches=false)
{
    int length;
    int best   = m_min_match - 1;
    int low    = pos - m_mask - 1;
    int found  = 0;
    int head   = hash(buf,pos,len);
    int next   = m_head[head];
    int n,m;

    if (low < 0) {
        low = 0;
    }

    while (next >= low && found < m_max_chain) {
        m = next;
        n = pos;
        length = 0;

        while (buf[m] == buf[n]) {
            ++m; ++n; ++length;

            if (length >= m_max_match) {
                break;
            }
        }

#if 1
        if (length >= m_min_match) {
            if (only_better_matches) {
                if (length <= best) {
                    continue;
                } else {
                    best = length;
                }
            }
            m_mtch[found].length = length;
            m_mtch[found].offset = pos-next;
            ++found;
        }
#else
        if (length > best) {
            best = length;
            m_mtch[found].length = length;
            m_mtch[found].offset = pos-next;
            ++found;
        }
#endif

        // Advance to the next history position in the hash chain
        next = m_next[next & m_mask];
    }
    
    // Update the hash chain with the current position..
    m_next[pos & m_mask] = m_head[head];
    m_head[head] = pos;

    return found;
}


#if 0
int main(int argc, char** argv)
{
    hash3_match h3(65536,2,256);

    return 0;
}


#endif
