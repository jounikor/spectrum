/**
 * @file bffp.cpp
 * @brief A Brute Force Forward Parser for LZ engines..
 * @version 0.1
 * @author Jouni Korhonen
 * @date 5-Feb-2024
 *
 *
 *
 */

#define LZ_PARSE_DEBUG

#include <iostream>
#include <new>
#include <cstring>
#include "bffp.h"
#include "lz_base.h"

#ifdef LZ_PARSE_DEBUG
#include <cctype>
#endif  // LZ_PARSE_DEBUG

/** 
 * Initialized statics that describe this LZ parser.
 */
const int bffp::min_match_length_s = 2;
const int bffp::max_match_length_s = 256;
const int bffp::num_window_sizes_s = sizeof(bffp::window_sizes_s)/sizeof(int);
const int bffp::window_sizes_s[2] = {1<<16, 1<<24};
const int bffp::max_buffer_length_s = 0;
const bool bffp::is_forward_parser_s = true;


/*
 *
 *
 */

void bffp::lz_reset_forward(void)
{
    m_forward_base = 0;
    m_forward_head = 0;
}

int bffp::lz_move_forward(int steps)
{
    m_forward_base = m_forward_base + steps;
    
    if (m_forward_base > m_forward_head) {
        m_forward_head = m_forward_base;
    }

    return m_forward_base;
}


bffp::bffp(int window_size, int min_match, int max_match, int num_fwd, int max_chain) :
    m_mtch(num_fwd,max_chain),
    m_lz(window_size,min_match,max_match),  // may throw exception
    lz_base()
{
    printf("bffp::bffp()\n");


    if (min_match < min_match_length_s) {
        EXCEPTION(std::length_error,"Minimum match length too small");
    }
    if (max_match > max_match_length_s) {
        EXCEPTION(std::length_error,"Maximum match length too big");
    }

    switch (window_size) {
    case 1<<16:
    case 1<<24:
        break;
    default:
        EXCEPTION(std::invalid_argument,"Invalid sliding window size");
    }

    m_min_match    = min_match;
    m_max_match    = max_match;
    m_window_size  = window_size;
    m_forward_head = 0;
    m_forward_base = 0;
}

bffp::~bffp(void)
{
    printf("bffp::~bffp(void)\n");
}

void bffp::lz_match_init(const char* buf, int len)
{
    m_buf = buf;
    m_buf_len = len;
}


const matches* bffp::lz_get_matches(void)
{
    return &m_mtch;
}


const matches& bffp::lz_forward_match(void)
{
    match *m;
    //uint32_t mask = m_mtch.get_max_forward_mask();
    int i,j;
    uint32_t fwd = m_mtch.get_max_forward();

    // Update the forward match lookup buffer if there is a need for it.
    // The minimum update is one step.
    if (m_forward_head < m_buf_len) {
        do {
            m = m_mtch[m_forward_head].matches;

            m_lz.init_get_matches(m_mtch.get_max_chain(),m);
            
            // Find all matches at this position. Returned 'i' is the index of
            // the longest found match.
            i = m_lz.find_matches(m_buf+m_forward_head,m_buf_len-m_forward_base,false);
#ifdef LZ_PARSE_DEBUG
            ::printf("%6d: '%c' (%02x) -> ",m_forward_head,
                ::isprint(m_buf[m_forward_head]) ? m_buf[m_forward_head] : ' ',
                m_buf[m_forward_head]);

                if (i >= 0) {
                    for (int n = 0; n < m_lz.get_num_found_matches(); n++) {
                        printf("%d:%d ",m[n].pos,m[n].len);
                    }
                }
                printf("\n");
#endif  // LZ_PARSE_DEBUG
            if (i >=0) {
                m_mtch[m_forward_head].num_matches = m_lz.get_num_found_matches();
                m_mtch[m_forward_head].max_match_index = i;
            } else {
                // No match i.e. literal
                m_mtch[m_forward_head].num_matches = 0;
                m_mtch[m_forward_head].found.literal = m_buf[m_forward_head];
            }
            ++m_forward_head;
        } while (m_forward_base+fwd > m_forward_head);
    }

    return m_mtch;
}




#if 1


template <typename Derived> const matches& interface_test(lz_base<Derived>* lz)
{
    return lz->lz_forward_match();
}



int main(int argc, char **argv)
{
    const char *buf = argv[1];
    int len = ::strlen(buf);
    int j, n,i;
    int m;

    //lz_cost *cost = new bffp_cost();

    printf("min_match_length: %d\n",bffp::min_match_length_s);
    printf("num_window_sizes: %d\n",bffp::num_window_sizes_s);
    for (n = 0; n < bffp::num_window_sizes_s; n++) {
        printf("> %d\n",bffp::window_sizes_s[n]);
    }

    lz_base<bffp> *bf = new bffp(65536,2,256,16,8);
    bf->lz_match_init(buf,len);

    for (n = 0; n < len; n++) {
        //const matches& mm = bf->lz_forward_match();
        const matches& mm = interface_test(bf);
        const matches* pp = bf->lz_get_matches();

        printf(">>mm %d\tpp %d\n",mm[n].num_matches,pp->at(n).num_matches);

        bf->lz_move_forward(1);
    }

    delete bf;
    //cost->cost(1,2);
    //delete cost;

    return 0;
}

#endif
