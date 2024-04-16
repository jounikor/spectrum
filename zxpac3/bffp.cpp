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
#include "bffp_cost.h"
#include "lz_util.h"

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

bffp::bffp(int window_size, int min_match, int max_match,
    int max_chain, int backward_steps,
    int p0, int p1, int p2, int p3,
    int ins, int res, int max, 
    int offset_ctx) :
    m_lz(window_size,min_match,max_match),  // may throw exception
    m_cost(backward_steps),
    m_cost_array(NULL),
    m_mtch(NULL),
    m_pmr(p0,p1,p2,p3),
    m_mtf(256,128,ins,res,max),
    lz_base()
{
    printf("bffp::bffp()\n");

    m_pmr.dump();

    //if (m_mtch == NULL) {
    //    EXCEPTION(std::bad_alloc,"Allocating memory for match array failed");
    //}
    //if (min_match < min_match_length_s) {
    //    //EXCEPTION(std::length_error,"Minimum match length too small");
    //}
    if (max_match > max_match_length_s) {
        //EXCEPTION(std::length_error,"Maximum match length too big");
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
    m_alloc_len  = 0;
    m_max_chain = max_chain;
    m_offset_ctx = offset_ctx;
}

bffp::~bffp(void)
{
    printf("bffp::~bffp(void)\n");
    //if (m_mtch) {
    //    delete[] m_mtch;
    //}
    //if (m_cost_array) {
    //    delete[] m_cost_array;
    //}
}


int bffp::lz_search_matches(const char* buf, int len, int interval)
{
    int pos = 0;
    int num;

#ifdef BFFP_DEBUG
    ::printf("-- Find all maches ------------------------------------------\n");
#endif  // BFFP_DEBUG

    while (pos < len) {
        m_lz.init_get_matches(m_max_chain,m_cost_array[pos].matches);
            
        // Find all matches at this position. Returned 'num' is the
        // the number of found matches.
        num = m_lz.find_matches(buf,pos,len-pos,false);
        m_cost_array[pos].num_matches = num;
#ifdef BFFP_DEBUG
        ::printf("%6d: '%c' (%02x) -> ",pos,
            ::isprint(buf[pos]) ? buf[pos] : ' ', buf[pos] & 0xff);
        if (num > 0) {
            for (int n = 0; n < num; n++) {
                printf("%d:%d ",m_cost_array[pos].matches[n].offset,
                    m_cost_array[pos].matches[n].length);
            }
        } else {
            ::printf("no match");
        }
        printf("\n");
#endif  // BFFP_DEBUG
        ++pos;
    }

    return 0;
}

int bffp::literal_pmr_available(int pos, cost_base* c, const char* buf)
{
    cost_with_ctx* p_ctx = static_cast<cost_with_ctx*>(c);
    int pmr_index;
    int offset;

    for (pmr_index = 0; pmr_index < PMR_MAX_NUM; pmr_index++) {
        offset = m_pmr.get(pmr_index);
        
        if (pos-offset >= 0) {
            if (p_ctx[pos-offset].literal == p_ctx[pos].literal) {
            // Here.. we replace aything worse or equal with a PMR.
#ifdef BFFP_DEBUG
                ::printf("**Literal PMR%d at %d (%d)\n",pmr_index,pos+offset,offset);
                m_pmr.dump();
#endif // BFFP_DEBUG
                return pmr_index;
            }
        }
    }
    return -1;
}


int bffp::match_pmr_available(int pos, cost_base* c, const char* buf) 
{
    cost_with_ctx* p_ctx = static_cast<cost_with_ctx*>(c);
    int offset = p_ctx[pos].offset;
    int length = p_ctx[pos].length;
    int pmr_index = m_pmr.find_pmr_by_offset(offset);

    // Sanity checks..
    assert(length > 1);
    assert(offset > 0);
    assert(offset < 16777216);

    if (pmr_index >= 0) {
#ifdef BFFP_DEBUG
        ::printf("Checking bffp_pmr%d at %d for %d:%d\n",pmr_index,pos,offset,length);
        ::printf("> bffp_pmr%d matches\n",pmr_index);
        m_pmr.dump();
#endif // BFFP_DEBUG
        return pmr_index;
    }

    return -1;
}



int bffp::lz_parse(const char* buf, int len, int interval)
{
    int rounds = 0;
    int length;
    int offset_ctx;
    int pos;
    int tag;
    int sta_pos;
    cost_base* cb = static_cast<cost_base*>(m_cost_array);
    uint8_t found;
    bool initial = true;

    // Phase 1 parsing.. just literals and simple offsets marking..
    sta_pos = 0;

    do {
        ::printf("-- Cost calculation #%d --------------------------------\n",
            rounds++);
        m_cost.init_cost(cb,sta_pos,len-sta_pos,initial);
        initial = false;
        pos = sta_pos;

        while (pos < len) {
            // always do literal cost calculation
            m_cost.impl_literal_cost(pos,m_cost_array,buf);

            // match cost calculation if not at the end of file and there was a match
            if (pos < (len - 2)) {
                m_cost.impl_match_cost(pos,m_cost_array,buf);
            }        

            ++pos;
        }

        pos = len;
        int num_literals = 1;

        // Fix the links of selected cost nodes
        m_cost_array[pos-length].next = 0;
        while (pos > 0) {
            length = m_cost_array[pos].length;
            assert(length > 0);
            m_cost_array[pos-length].next = pos;
            
            if (m_cost_array[pos].tag == lz_tag_type::bffp_lit) {
                m_cost_array[pos].num_literals = num_literals++;
            }
            if (m_cost_array[pos].tag != lz_tag_type::bffp_lit) {
                num_literals = 1;
                m_cost_array[pos].num_literals = 0;
            }

            pos -= length;
        }

#ifdef BFFP_DEBUG
        ::printf("-- Cost debugging phase ----------------------------------\n");
        pos = 1;

        while (pos < len+1) {
            ::printf("%6d: %d '%c' (%02x) %d %d:%d %u -> %d\n",pos,
                m_cost_array[pos].tag,
                ::isprint(m_cost_array[pos].literal) ? m_cost_array[pos].literal : ' ',
                m_cost_array[pos].literal,
                m_cost_array[pos].num_literals,
                m_cost_array[pos].offset,m_cost_array[pos].length,
                m_cost_array[pos].arrival_cost,
                m_cost_array[pos].next);
            ++pos;
        }
#endif // BFFP_DEBUG
        ::printf("-- Cost optimation phase ---------------------------------\n");
    } while (rounds++ < 0 && sta_pos < len);

    return 0;
}

cost_base* bffp::lz_cost_array_get(int len)
{
    if (len < 1) {
       return NULL;
    }

    if (len < m_alloc_len && m_cost_array) {
        return m_cost_array;
    }

    m_cost_array = static_cast<cost_with_ctx*>(m_cost.alloc_cost(len,m_max_chain)); 
    m_alloc_len = len;

    return m_cost_array;
}

void bffp::lz_cost_array_done(void)
{
    m_cost.free_cost(m_cost_array); 
    m_alloc_len = 0;
    m_cost_array = NULL;


}

#if 1


int main(int argc, char **argv)
{
    const char *buf = argv[1];
    int len = ::strlen(buf);
    int j, n,i;
    int m;
    int chain = 8;
    int pos;

    printf("min_match_length: %d\n",bffp::min_match_length_s);
    printf("num_window_sizes: %d\n",bffp::num_window_sizes_s);
    for (n = 0; n < bffp::num_window_sizes_s; n++) {
        printf("> %d\n",bffp::window_sizes_s[n]);
    }

    bffp *bf = new bffp(65536,2,256,chain, 0, 4,7,21,32,96,5,0, 256);
    cost_base* my_cost = bf->lz_cost_array_get(len);

    bf->lz_search_matches(buf,len,0); 
    bf->lz_parse(buf,len,0); 

    
    bf->lz_cost_array_done();

    delete bf;
    return 0;
}

#endif
