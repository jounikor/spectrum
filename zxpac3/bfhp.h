#ifndef _BFHP_H_INCLUDED
#define _BFHP_H_INCLUDED

#include <vector>
#include <cstdint>
#include "lz_base.h"
#include "hash.h"
#include "bfhp_cost.h"


#define MTF256_MAX_QUEUE 4

/**
 * @class matches utils.h
 * @brief A container class to hold all matches within the sliding window.
 * 
 *
 */

class bfhp : public lz_base<bfhp> {
    int m_max_chain;
    int m_min_match;
    int m_max_match;
    int m_window_size;      // SHALL be power of 2
    hash3_match m_lz;
    bfhp_cost m_cost;
    cost_with_ctx* m_cost_array;
    int m_alloc_len;
    mtf_encode m_mtf;       ///< MTF256 object for literal encoding
    int m_offset_ctx;
    int m_pmr_offsets[PMR_MAX_NUM];
    bool m_default_pmr;
    bool m_debug;
    bool m_verbose;
public:
    bfhp(int window_size, int min_match, int max_match,
        int max_chain, int backward_steps,
        int ins, int res, int max, 
        int offset_ctx,
        bool default_pmr=false);
    ~bfhp();
    int lz_search_matches(const char* buf, int len, int interval);
    int lz_parse(const char* buf, int len, int interval);
    int lz_encode(const cost_base* costs, uint32_t size, std::ofstream& ofs);

    cost_base* lz_get_result(void) { return m_cost_array; }
    cost_base* lz_cost_array_get(int len);
    void lz_cost_array_done(void);

    // Debug interface
    void lz_enable_debug(bool enable);
    void lz_enable_verbose(bool enable);

    // Standard interface for the LZ-parser capabilities
    static const int min_match_length_s;
    static const int max_match_length_s;
    static const int num_window_sizes_s;
    static const int window_sizes_s[2];
    static const int max_buffer_length_s;
    static const bool is_forward_parser_s;
};



#endif  // _BFHP_H_INCLUDED
