/**
 * @file cost4b.h
 * @version 0.2
 * @brief lzpac4 specific Literal & Match encoding cost calculator.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 7-Apr-2024
 * @copyright The Unlicense 
 *
 */

#ifndef _COST4B_INCLUDED
#define _COST4B_INCLUDED

#include "lz_base.h"

class zxpac4b_cost: public lz_cost<zxpac4b_cost> {
    bool m_debug;
    bool m_verbose;
public:
   zxpac4b_cost(
        const lz_config* p_cfg, int ins=-1, int max=-1);
    ~zxpac4b_cost(void);

    // Base class interface method implementations
    int impl_literal_cost(int pos, cost* c, const char* buf);
    int impl_match_cost(int pos, cost* c, const char* buf);
    int impl_init_cost(cost* c, int sta, int len, int pmr);
    cost* impl_alloc_cost(int len, int max_chain);
    int impl_free_cost(cost* cost);

    int impl_get_offset_bits(int offset);
    int impl_get_length_bits(int length);
    int impl_get_literal_bits(char literal, bool is_ascii);
    
    int impl_get_offset_tag(int offset, char& byte_tag, int& bit_tag);
    int impl_get_length_tag(int length, int& bit_tag);
    int impl_get_literal_tag(const char* literals, int length, bool is_ascii, char& byte_tag, int& bit_tag);

    void impl_enable_debug(bool enable) {
        m_debug = enable;
    }
    void impl_enable_verbose(bool enable) {
        m_verbose = enable;
    }
};

#endif  // _COST4B_INCLUDED
