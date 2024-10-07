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
#include "lz_util.h"

/**
 * \def LZ_MAX_COST     Maximum cost for LZ cost calculations. Used to initialize
 */
#define LZ_MAX_COST             0x7fffffff

/**
 *
 */



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

struct lz_config {
    int window_size;            // power of two allowed
    int max_chain;
    int min_match;
    int max_match;
    int good_match;
    int backward_steps;         // currently not used
    int min_match2_threshold;   // currently not used
    int min_match3_threshold;   // currently not used
    int initial_pmr_offset;
    //
    bool only_better_matches;
    bool is_ascii;
    //bool forward_parse;       // DEPRECATED
    bool reversed_file;
};

/**
 *
 * @brief A base template class for a LZ parser cost calculations.
 */

template <typename Derived> class lz_cost {
    // This indirection gets inlined by the compiler anyway..
    Derived& impl(void) {
        return *static_cast<Derived*>(this);
    }
protected:
    const lz_config* m_lz_config;
    int m_max_len;

    int check_match(const char* s, const char* d, int max) {
        int len = 0;
        while (*s++ == *d++ && len++ < max); 
        return len;
    }

public:
    lz_cost(const lz_config* p_cfg): m_lz_config(p_cfg) {}
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
    void enable_debug(bool enable) {
        impl().impl_enable_debug(enable);
    }
    void enable_verbose(bool enable) {
        impl().impl_enable_verbose(enable);
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
    int get_literal_tag(const char* literals, int length, bool is_ascii, char& byte_tag, int& bit_tag) {
        return impl().impl_get_literal_tag(literals,length,is_ascii,byte_tag,bit_tag);
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
    bool m_debug;
    bool m_verbose;
public:
    lz_base(const lz_config* p_cfg): m_lz_config(p_cfg), m_debug(false), m_verbose(false) { }
    virtual ~lz_base(void) { }

    // The interface definition for the base LZ class..
    const lz_config* lz_get_config(void) {
        return m_lz_config;
    }
    virtual int lz_search_matches(const char* buf, int len, int interval) = 0;
    virtual int lz_parse(const char* buf, int len, int interval) = 0;
    virtual const cost* lz_get_result(void) = 0;
    virtual const cost* lz_cost_array_get(int len) = 0;
    virtual void lz_cost_array_done(void) = 0;
    virtual int lz_encode(const char* buf, int len, std::ofstream& ofs) = 0;
    void enable_debug(bool enable) {
        m_debug = enable;
    }
    void enable_verbose(bool enable) {
        m_verbose = enable;
    }
    bool debug(void) {
        return m_debug;
    }
    bool verbose(void) {
        return m_verbose;
    }

    // Simple statistics
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
};


#endif  // _LZ_BASE_H_INCLUDED
