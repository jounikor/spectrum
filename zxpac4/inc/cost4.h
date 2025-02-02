/**
 * @file cost.h
 * @version 0.2
 * @brief lzpac4 specific Literal & Match encoding cost calculator.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 7-Apr-2024
 * @copyright The Unlicense 
 *
 */

#ifndef _COST_INCLUDED
#define _COST_INCLUDED

#include "lz_base.h"
#include "mtf256.h"

#ifdef MTF_CTX_SAVING_ENABLED
#define ZXPAC4_MTF_CTX_SIZE   128   // This is a bit of dirty forward knowledge how
                                    // mtf256 works.. just to avoid dynamic memory
                                    // allocation for all contexts..
#endif

class zxpac4_cost: public lz_cost<zxpac4_cost> {
    //mtf_encode m_mtf;
    bool m_debug;
    bool m_verbose;
public:
   zxpac4_cost(
        const lz_config* p_cfg, int ins=-1, int max=-1);
    ~zxpac4_cost(void);

    // Base class interface method implementations
    int impl_literal_cost(int pos, cost* c, const char* buf);
    int impl_match_cost(int pos, cost* c, const char* buf, int offset, int length);
    int impl_init_cost(cost* c, int sta, int len, int pmr);
    cost* impl_alloc_cost(int len, int max_chain);
    int impl_free_cost(cost* cost);

    int impl_get_offset_bits(int offset);
    int impl_get_length_bits(int length);
    int impl_get_literal_bits(char literal, bool is_ascii);
    
    int impl_get_offset_tag(int offset, char& byte_tag, int& bit_tag);
    int impl_get_length_tag(int length, int& bit_tag);
    int impl_get_literal_tag(const char* literals, int length, char& byte_tag, int& bit_tag);
    
    void impl_enable_debug(bool enable) {
        m_debug = enable;
    }
    void impl_enable_verbose(bool enable) {
        m_verbose = enable;
    }
};

#endif  // _COST_INCLUDED
