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

#include "lz_util.h"

/**
 * @brief A structure to hold match-length pairs for a LZ string matching engine.
 * @struct match z_alg.h
 */


#define LZ_MAX_COST 0xffffffff

struct cost_base {
    int next;
    int tag;
    int offset;
    char literal;
    char escape;
    uint16_t length;
    uint32_t arrival_cost;
};


template <typename Derived> class lz_cost {
    // This indirection gets inlined by the compiler anyway..
    Derived& impl(void) {
        return *static_cast<Derived*>(this);
    }
public:
    lz_cost() {}
    virtual ~lz_cost() {}

    int literal_cost(int pos, cost_base* c, const char* buf, int weight) {
        return impl().impl_literal_cost(pos,c,buf,weight);
    }
    int match_cost(int pos, match* m, int len, cost_base* c, uint8_t* buf) {
        return impl().impl_matchl_cost(pos,m,len,c,buf);
    }
    int init_cost(cost_base* c, int sta, int len, bool initial) {
        return impl().impl_init_cost(c,sta,len,initial);
    }
    cost_base* alloc_cost(int len, int max_chain) {
        return impl().impl_alloc_cost(len,max_chain);
    }
    int free_cost(cost_base* cost) {
        return impl().impl_free_cost(cost);
    }
};


/**
 * @brief A CRTP base template class for a generic LZ string searching engine.
 * @class lz_base lz_base.h
 *
 * Trying the "Curiously recurring template pattern" (CRTP).
 */
template <typename Derived> class lz_base {
private:
    // This indirection gets inlined by the compiler anyway..
    Derived& impl(void) {
        return *static_cast<Derived*>(this);
    }
public:
    lz_base(void) { }
    virtual ~lz_base(void) { }

    // The interface definition for the base LZ class..
    int lz_search_macthes(const char* buf, int len, int interval)
    {
        return impl().lz_search_matches(buf,len,interval);
    }
    int lz_parse(const char* buf, int len, int interval)
    {
        return impl().lz_parse(buf,len,interval);
    }
    const cost_base* lz_get_result(void) {
        return impl().lz_get_result();
    }
    cost_base* lz_cost_array_get(int len) {
        return impl().lz_cost_array_get(len);
    }
    void lz_cost_array_done(void) {
        return impl().lz_cost_array_done();
    }
};


#endif  // _LZ_BASE_H_INCLUDED
