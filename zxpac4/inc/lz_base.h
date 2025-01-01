/**
 * @file lz_base.h
 * @brief A framework for a generic LZ string searching engine.
 * @version 0.2
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 11-Apr-2022
 * @date 10-Mar-2024
 * @copyright The Unlicense
 *
 * This file implements a CRTP base template classes for a generic LZ string
 * searching engine.
 */
#ifndef _LZ_BASE_H_INCLUDED
#define _LZ_BASE_H_INCLUDED

#include <iostream>
#include "cstdint"
#include "lz_util.h"

/**
 * \def LZ_MAX_COST     Maximum cost for LZ cost calculations. Used to initialize
 */
#define LZ_MAX_COST             0x7fffffff

/**
 *
 */

#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_NORMAL  1
#define DEBUG_LEVEL_EXTRA   2

/**
 * @struct cost lz_base.h inc/lz_base.h
 * @brief A structure to hold match-length pairs for a LZ string matching engine.
 */

struct cost {
    int32_t next;
    int32_t offset;
    int32_t length;
    uint32_t arrival_cost;
    int32_t pmr_offset;         ///< tbd
    int16_t num_matches;        ///< Number of matches in @p matches array
    int16_t num_literals;       ///< Number of consequtive literal up to this match node.
    match *matches;         ///< An array of matches
    bool last_was_literal;
};

/**
 * @struct lz_config lz_base.h inc/lz_base.h
 * @brief A structure containing basic LZ engine configurations.
 */

typedef struct lz_config {
    int window_size;            // power of two allowed
    int max_chain;
    int min_match;
    int max_match;
    int good_match;
    int backward_steps;         // currently not used
    int min_match2_threshold;   // currently not used
    int min_match3_threshold;   // currently not used
    int initial_pmr_offset;
    int debug_level;            // 
    int algorithm;
    //
    bool only_better_matches:1;
    bool reverse_file:1;
    bool reverse_encoded:1;
    bool is_ascii:1;
    bool preshift_last_ascii_literal:1;
    bool merge_hunks:1;         // Amiga specific
    bool verbose:1;
} lz_config_t;

/**
 *
 * @brief A base template class for a LZ parser cost calculations.
 */

template <typename Derived> class lz_cost {
    // This indirection gets inlined by the compiler anyway..
    Derived& impl(void) {
        return *static_cast<Derived*>(this);
    }
    int m_debug_level;
    bool m_verbose;
    const lz_config* m_lz_config;
protected:
    int m_max_len;

    int check_match(const char* s, const char* d, int max) {
        int len = 0;
        while (*s++ == *d++ && ++len < max); 
        return len;
    }

public:
    lz_cost(const lz_config* p_cfg):
        m_debug_level(DEBUG_LEVEL_NONE), 
        m_verbose(false),
        m_lz_config(p_cfg) {}
    virtual ~lz_cost() {}

    const lz_config* lz_get_config(void) {
        return m_lz_config;
    }
    int literal_cost(int pos, cost* c, const char* buf) {
        return impl().impl_literal_cost(pos,c,buf);
    }
    int match_cost(int pos, cost* c, const char* buf) {
        return impl().impl_match_cost(pos,c,buf);
    }
    int init_cost(cost* c, int sta, int len, int pmr) {
        return impl().impl_init_cost(c,sta,len,pmr);
    }
    cost* alloc_cost(int len, int max_chain) {
        return impl().impl_alloc_cost(len,max_chain);
    }
    int free_cost(cost* cost) {
        return impl().impl_free_cost(cost);
    }
    int get_offset_bits(int offset) {
        return impl().impl_get_offset_bits(offset);
    }
    int get_length_bits(int length) {
        return impl().impl_get_length_bits(length);
    }
    int get_literal_bits(char literal, bool is_ascii) {
        return impl().impl_get_literal_bits(literal,is_ascii);
    }
    int get_offset_tag(int offset, char& byte_tag, int& bit_tag) {
        return impl().impl_get_offset_tag(offset,byte_tag,bit_tag);
    }
    int get_length_tag(int length, int& bit_tag) {
        return impl().impl_get_length_tag(length,bit_tag);
    }
    int get_literal_tag(const char* literals, int length, char& byte_tag, int& bit_tag) {
        return impl().impl_get_literal_tag(literals,length,byte_tag,bit_tag);
    }
    
    // Implementation within the base class
    void set_debug_level(int level) {
        m_debug_level = level;
    }
    int get_debug_level(void) {
        return m_debug_level;
    }
    void enable_verbose(bool enable) {
        m_verbose = enable;
    }
    bool verbose(void) {
        return m_verbose;
    }
};


/**
 * @brief A CRTP base template class for a generic LZ string searching engine.
 * @class lz_base lz_base.h
 *
 * Trying the "Curiously recurring template pattern" (CRTP).
 */
class lz_base {
protected:
    // statistics
    int m_num_literals;
    int m_num_pmr_literals;
    int m_num_matches;
    int m_num_matched_bytes;
    int m_num_pmr_matches;
    // debugs and configs
    const lz_config* m_lz_config;
    int m_debug_level;
    bool m_verbose;
    int m_security_distance;
protected:
    void reverse_buffer(char* p_buf, int len) {
        char t;
        for (int n = 0; n < len/2; n++) {
            t = p_buf[n];
            p_buf[n] = p_buf[len-n-1];
            p_buf[len-n-1] = t;
        }
    }
public:
    lz_base(const lz_config* p_cfg): m_lz_config(p_cfg), 
        m_debug_level(DEBUG_LEVEL_NONE), 
        m_verbose(false),
        m_security_distance(0) { }
    virtual ~lz_base(void) { }

    // The interface definition for the base LZ class..
    const lz_config* lz_get_config(void) {
        return m_lz_config;
    }
    virtual int lz_search_matches(char* buf, int len, int interval) = 0;
    virtual int lz_parse(const char* buf, int len, int interval) = 0;
    virtual const cost* lz_get_result(void) = 0;
    virtual const cost* lz_cost_array_get(int len) = 0;
    virtual void lz_cost_array_done(void) = 0;
    virtual int lz_encode(const char* buf, int len, std::ofstream& ofs) = 0;
    
    // Methods implemented within the base class
    void set_debug_level(int level) {
        m_debug_level = level;
    }
    int get_debug_level(void) {
        return m_debug_level;
    }
    void enable_verbose(bool enable) {
        m_verbose = enable;
    }
    bool verbose(void) {
        return m_verbose;
    }
    int get_num_literals(void) const {
        return m_num_literals;
    }
    int get_num_pmr_literals(void) const {
        return m_num_pmr_literals;
    }
    int get_num_matches(void) const {
        return m_num_matches;
    }
    int get_num_matched_bytes(void) const {
        return m_num_matched_bytes;
    }
    int get_num_pmr_matches(void) const {
        return m_num_pmr_matches;
    }
    int get_security_distance(void) const {
        return m_security_distance;
    }
};


#endif  // _LZ_BASE_H_INCLUDED
