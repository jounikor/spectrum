#ifndef _BFFP_H_INCLUDED
#define _BFFP_H_INCLUDED

#include <vector>
#include <cstdint>
#include "lz_base.h"
//#include "z_alg.h"
#include "hash.h"
#include "bffp_cost.h"

/**
 * @class matches utils.h
 * @brief A container class to hold all matches within the sliding window.
 * 
 *
 */

class bffp : public lz_base<bffp> {
    int m_max_chain;
    int m_min_match;
    int m_max_match;
    int m_window_size;      // SHALL be power of 2
    int m_backward_steps;
    cost_base* m_mtch;
    //match* m_mtch;
    // The forward search routines
    //z_array m_lz;
    hash3_match m_lz;
    bffp_cost m_cost;
    cost_with_ctx* m_cost_array;
    int m_alloc_len;
    pmr m_pmr;              ///< PMR object for PMRs
    mtf_encode m_mtf;       ///< MTF256 object for literal encoding
    int m_offset_ctx;
public:
    bffp(int window_size, int min_match, int max_match,
        int max_chain, int backward_steps,
        int p0, int p1, int p2, int p3,
        int ins, int res, int max, 
        int offset_ctx);
    ~bffp();
    int lz_search_matches(const char* buf, int len, int interval);
    int lz_parse(const char* buf, int len, int interval);
    cost_base* lz_get_result(void) { return m_cost_array; }
    cost_base* lz_cost_array_get(int len);
    void lz_cost_array_done(void);
    
    //

    int literal_pmr_available(int pos, cost_base* c, const char* buf);
    int match_pmr_available(int pos, cost_base* c, const char* buf);

    // Standard interface for the LZ-parser capabilities
    static const int min_match_length_s;
    static const int max_match_length_s;
    static const int num_window_sizes_s;
    static const int window_sizes_s[2];
    static const int max_buffer_length_s;
    static const bool is_forward_parser_s;
};



#endif  // _BFFP_H_INCLUDED
