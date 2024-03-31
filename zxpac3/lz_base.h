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

#include "lz_utils.h"

typedef struct cost_ {
    int tag;
    int len;
    int cost;
} cost_t;



template <typename Derived> class lz_cost {
    // This indirection gets inlined by the compiler anyway..
    Derived& impl(void) {
        return *static_cast<Derived*>(this);
    }
public:
    lz_cost() {}
    virtual ~lz_cost() {}
    int cost(int off, int len, cost_t& estimated_cost)
    {
        return impl().cost(off,len,estimated_cost);
    }
    void* state_save(void *ctx)
    {
        return impl().state_save(ctx);
    }
    void *get_state(void) {
        return impl().get_state();
    }
    void state_load(void *ctx)
    {
        impl().state_load(ctx);
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
    lz_base() { }
    virtual ~lz_base(void) { }

    // The interface definition for the base LZ class..
    void lz_match_init(const char* buf, int len)
    {   
        impl().lz_match_init(buf,len);
    }
    const matches& lz_forward_match(void)
    {
        return impl().lz_forward_match();
    }
    const matches* lz_get_matches(void)
    {
        return impl().lz_get_matches();
    }
    void lz_reset_forward(void)
    {
        impl().lz_reset_forward();
    }
    int lz_move_forward(int steps)
    {
        return impl().lz_move_forward(steps);
    }
};

/**
 * @brief A LZ parser base class and API
 *
 *
 *
 */
template <typename Derived> class lz_parse {
    Derived& impl(void) {
        return *static_cast<Derived*>(this);
    }
public:
    lz_parse(void) {}
    virtual ~lz_parse(void) {}

    // The interface definition


};



#endif  // _LZ_BASE_H_INCLUDED
