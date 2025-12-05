/**
 * @file cost4c.h
 * @version 0.3
 * @brief lzpac4c specific Literal & Match encoding cost calculator.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 7-Jul-2025
 * @copyright The Unlicense 
 *
 */

#ifndef _COST4C_H_INCLUDED
#define _COST4C_H_INCLUDED

#include "lz_base.h"
#include "tans_encoder.h"

// tANS specific information..

#define TANS_NUM_LITERAL_SYM    10
#define TANS_NUM_MATCH_SYM      10
#define TANS_NUM_OFFSET_SYM     10
#define TANS_SIZE_LITERAL       32
#define TANS_SIZE_MATCH         32
#define TANS_SIZE_OFFSET        32


#define TANS_LITERAL_RUN_SYMS   0
#define TANS_LENGTH_SYMS        1
#define TANS_OFFSET_SYMS        2


class zxpac4c_cost: public lz_cost<zxpac4c_cost> {
    bool m_debug;
    bool m_verbose;

    // tANS symbol frequencies
    int m_literal_sym_freq[TANS_NUM_LITERAL_SYM];
    int m_match_sym_freq[TANS_NUM_MATCH_SYM];
    int m_offset_sym_freq[TANS_NUM_OFFSET_SYM];

    // static size tANS classes..
    tans_encoder<uint8_t,TANS_SIZE_LITERAL> m_tans_literal;
    tans_encoder<uint8_t,TANS_SIZE_MATCH> m_tans_match;
    tans_encoder<uint8_t,TANS_SIZE_OFFSET> m_tans_offset;
public:
   zxpac4c_cost(
        const lz_config* p_cfg, int ins=-1, int max=-1);
    ~zxpac4c_cost(void);

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

    // New API specific to zxpac4c
    ans_state_t get_tans_state(int type);    
    void build_tans_tables(void);
    int inc_tans_symbol_freq(int type, uint8_t symbol);
    void set_tans_symbol_freqs(int type, uint8_t* freqs=NULL, int len=0);
    const int* get_tans_scaled_symbol_freqs(int type, int& len);

    int predict_tans_cost(int type, int value);
    void dump(int type);

};

#endif  // _COST4C_H_INCLUDED
