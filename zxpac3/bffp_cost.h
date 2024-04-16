/**
 * @file bffp_cost.h
 * @version 0.1
 * @brief BFFP specific Literal & Match encoding cost calculator.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 7-Apr-2024
 * @copyright The Unlicense 
 *
 * See the algorithms.txt for encodings. Note that context updating for
 * Literal encoding (MTF256) is not enabled, since every context would
 * take 128 bytes of memory. This implies the literal cost calculation
 * is somewhat off most of the time.
 */

#ifndef _BFFP_COST_INCLUDED
#define _BFFP_COST_INCLUDED

#include "lz_base.h"
#include "pmr.h"
#include "mtf256.h"

#define BFFP_PMR_CTX_SIZE   PMR_CTX_SIZE
#ifdef MTF_CTX_SAVING_ENABLED
#define BFFP_MTF_CTX_SIZE   128     // This is a bit of dirty forward knowledge how
                                    // mtf256 works.. jjst to avoid dynamic memory
                                    // allocation for all contexts..
#endif

// Encoding specific.. extend as needed, you won't run of numbers ;)
enum lz_tag_type {
    none,               // No match tag assigned.
    // bffp
    bffp_lit,           // tag 0LLLLLLL
    bffp_lit2,          // tag 0EEEEEEE + LLLLLLLL  // escaped literal
    bffp_pmr0,          // tag 10ppnnnn + opt [8]
    bffp_pmr1,          // tag 10ppnnnn + opt [8]
    bffp_pmr2,          // tag 10ppnnnn + opt [8]
    bffp_pmr3,          // tag 10ppnnnn + opt [8]
    bffp_mml2_off_ctx,  // tag 1100nnnn + opt [8] + off [8]
    bffp_mml2_off8,     // tag 1101nnnn + opt [8] + off [8]
    bffp_mml2_off16,    // tag 1110nnnn + opt [8] + off [8] + off [8]
    bffp_mml2_off24     // tag 1111nnnn + opt [8] + off [8] + off [8] + off [8]
};

struct cost_with_ctx : public cost_base {
    int16_t num_literals;   ///< Size of literal run..
    int16_t num_matches;    ///< Number of matches in @p matches array
    match *matches;         ///< An array of matches
};

class bffp_cost: public lz_cost<bffp_cost> {
    int m_backward_steps;   ///< Stored bakward steps
    int m_offset_ctx;       ///< Stored initial long offset context
    bool m_forward;         ///< Whether offset is to hostory or future
    int max_chain;
public:
    bffp_cost(
        int bs=1);          ///< Number of of backward steps for the LZ parser
    ~bffp_cost(void);

    // Base class interface method implementations
    int impl_literal_cost(int pos, cost_base* c, const char* buf);
    int impl_match_cost(int pos, cost_base* c, const char* buf);
    int impl_init_cost(cost_base* c, int sta, int len, bool initial);
    cost_base* impl_alloc_cost(int len, int max_chain);
    int impl_free_cost(cost_base* cost);
    //
};


#endif  // _BFFP_COST_INCLUDED
