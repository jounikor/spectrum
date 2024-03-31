#ifndef _BFFP_H_INCLUDED
#define _BFFP_H_INCLUDED

#include <vector>
#include <cstdint>
#include "lz_utils.h"
#include "lz_base.h"
#include "z_alg.h"

/**
 * @class matches utils.h
 * @brief A container class to hold all matches within the sliding window.
 * 
 *
 */

class bffp : public lz_base<bffp> {
    int m_min_match;
    int m_max_match;
    int m_window_size;      // should be power of 2
    uint32_t m_forward_head;
    uint32_t m_forward_base;
    uint32_t m_buf_len;
    const char *m_buf;
    matches m_mtch;
    // The forward search routines
    z_array m_lz;
public:
    bffp(int window_size, int min_match, int max_match, int num_fwd, int max_chain);
    ~bffp();
    void lz_match_init(const char* buf, int len);
    const matches& lz_forward_match(void);
    const matches* lz_get_matches(void);
    void lz_reset_forward(void); 
    int lz_move_forward(int steps);

    // Standard interface for the LZ-parser capabilities
    static const int min_match_length_s;
    static const int max_match_length_s;
    static const int num_window_sizes_s;
    static const int window_sizes_s[2];
    static const int max_buffer_length_s;
    static const bool is_forward_parser_s;
};



#endif  // _BFFP_H_INCLUDED
