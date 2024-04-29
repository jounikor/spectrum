/**
 * @file bfhp_cost.h
 * @version 0.2
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

#ifndef _BFHP_COST_INCLUDED
#define _BFHP_COST_INCLUDED

#include "lz_base.h"
#include "pmr.h"
#include "mtf256.h"

#define BFHP_PMR_CTX_SIZE   PMR_CTX_SIZE
#ifdef MTF_CTX_SAVING_ENABLED
#define BFHP_MTF_CTX_SIZE   128     // This is a bit of dirty forward knowledge how
                                    // mtf256 works.. jjst to avoid dynamic memory
                                    // allocation for all contexts..
#endif

#define INTIAL_PMR_DIST     1024
#define DEFAULT_P0          4
#define DEFAULT_P1          8
#define DEFAULT_P2          12
#define DEFAULT_P3          16

// Encoding specific.. extend as needed, you won't run of numbers ;)
enum lz_tag_type {
    none,               // No match tag assigned.
    // bfhp
    bfhp_lit,           // tag 0LLLLLLL
    bfhp_lit2,          // tag 0EEEEEEE + LLLLLLLL  // escaped literal
    bfhp_pmr0,          // tag 10ppnnnn + opt [8]
    bfhp_pmr1,          // tag 10ppnnnn + opt [8]
    bfhp_pmr2,          // tag 10ppnnnn + opt [8]
    bfhp_pmr3,          // tag 10ppnnnn + opt [8]
    bfhp_mml2_off_ctx,  // tag 1100nnnn + opt [8] + off [8]
    bfhp_mml2_off8,     // tag 1101nnnn + opt [8] + off [8]
    bfhp_mml2_off16,    // tag 1110nnnn + opt [8] + off [8] + off [8]
    bfhp_mml2_off24     // tag 1111nnnn + opt [8] + off [8] + off [8] + off [8]
};

struct cost_with_ctx : public cost_base {
    int num_matches;        ///< Number of matches in @p matches array
    int num_literals;       ///< Number of consequtive literal up to this match node.
    match *matches;         ///< An array of matches
    int offset_ctx;         ///< The offset context in this match node.
    pmr_ctx_t pmr_ctx[PMR_CTX_SIZE];    ///< The PMR context in this match node.
};

class bfhp_cost: public lz_cost<bfhp_cost> {
    int m_backward_steps;   ///< Stored bakward steps
    int32_t m_offset_ctx;
    pmr m_pmr;
    mtf_encode m_mtf;
    bool m_debug;
    bool m_verbose;
public:
    bfhp_cost(
        int p0,
        int p1,
        int p2,
        int p3,
        int ins,
        int res,
        int max,
        int oc,
        int bs=1);          ///< Number of of backward steps for the LZ parser
    ~bfhp_cost(void);

    // Base class interface method implementations
    int impl_literal_cost(int pos, cost_base* c, const char* buf);
    int impl_match_cost(int pos, cost_base* c, const char* buf);
    int impl_init_cost(cost_base* c, int sta, int len, bool initial);
    cost_base* impl_alloc_cost(int len, int max_chain);
    int impl_free_cost(cost_base* cost);
    void impl_enable_debug(bool enable) {
        m_debug = enable;
    }
    void impl_enable_verbose(bool enable) {
        m_verbose = enable;
    }
    //
    void calc_initial_pmr8(cost_with_ctx* p_ctx, int* p0_3, int len);
    void pmr_reset(int* pmrs) {
        m_pmr.reset(pmrs[0],pmrs[1],pmrs[2],pmrs[3]);
    }
};


#endif  // _BFHP_COST_INCLUDED
