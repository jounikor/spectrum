/**
 * @file bfhp_cost.cpp
 * @brief Calculates an approximate cost for the LZ encoding. This class
 *        is also aware of the used encoding and may maintain a state for
 *        PMRs.
 * @version 0.1
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 7-Apr-2024
 * @copyrigth The Unlicense
 *
 */

#include <iostream>
#include <cassert>
#include <stdint.h>
#include "bfhp_cost.h"

/**
 * @brief The defaulr constructor for the bfhp_cost class. 
 * @details tbd
 * 
 * @param[in] p0   PMR0 initial offset value.
 * @param[in] p1   PMR1 initial offset value.
 * @param[in] p2   PMR2 initial offset value.
 * @param[in] p3   PMR3 initial offset value.
 * @param[in] ins  MTF256 new code insert position (must be between 0
 *                 and 128-@p res).
 * @param[in] res  The number of MTF256 reserved code points for escapes.
 * @param[in] max  The max number of MTF moves. 0 = always to first.
 * @param[in] oc   The initial long offset context. Should be between
 *                 0x01 and 0xffff. The value is the "offset>>8".
 * @param[in] bs   The number of backward steps for better match checking.
 *                 Value 0 means only the 'match_length' of the match is
 *                 considered. Value 1 would mean the 'match_length' and
 *                 'match_length-1' are considered and so on. The values
 *                 must be between 0 and 254. In practise the value should
 *                 as small as possible.
 *
 * @return None.
 *
 * @note The constructor may throw exceptions.
 */

bfhp_cost::bfhp_cost(
    int p0, int p1, int p2, int p3,     ///< PMR initial values
    int ins, int res, int max,          ///< MTF intialization values
    int oc,                             ///< Initial offset context
    int bs) :                           ///< Number of backward steps
    m_backward_steps(bs),
    m_offset_ctx(oc),
    m_pmr(p0,p1,p2,p3),
    m_mtf(256,128,ins,res,max)
{
    if (bs < 0 || bs > 256-2) {
        EXCEPTION(std::out_of_range,"Backward steps must be > 0 and < 254");
    }

    m_debug = false;
    m_verbose = false;
}

bfhp_cost::~bfhp_cost()
{
}

/**
 * @brief Calculate the cost for encoding a literal character at the
 *        file @p pos in the @p buf.
 *
 * @param[in] pos  The current absolute position in the input buffer.
 * @param[inout] c A ptr to the array of costs where we calculate
 *                 the arrival cost for each position of the file
 *                 to be compressed.
 * @param[in] buf  A ptr to the input buffer (this is practically the
 *                 whole file to be compressed).
 *
 * @return The calculated cost for the literal at @p pos of the input buffer.
 */


int bfhp_cost::impl_literal_cost(int pos, cost_base* c, const char* buf)
{
    int offset;
    int pmr_index;
    int tag = lz_tag_type::none;
    cost_with_ctx* p_ctx = static_cast<cost_with_ctx*>(c);
    uint32_t new_cost = p_ctx[pos].arrival_cost;

    // First check PMR for this match node
    m_pmr.state_load(p_ctx[pos].pmr_ctx);

    for (pmr_index = 0; pmr_index < PMR_MAX_NUM; pmr_index++) {
        offset = m_pmr.get(pmr_index);
        
        if (pos-offset >= 0) {
            if (buf[pos-offset] == buf[pos]) {
                // Here.. we replace aything worse or equal with a PMR.
                // Note, we do not update PMR even if it used..
                new_cost += 8;
                tag = lz_tag_type::bfhp_pmr0 + pmr_index;
                break;
            }
        }
    }

    // Normal literal cost if no PMR was found
    if (tag == lz_tag_type::none) {
        // Lets bias this by one bit.. in reality this is either 8 or 16
        new_cost += 9;
        tag = lz_tag_type::bfhp_lit;
        offset = 0;
    }
    if (p_ctx[pos+1].arrival_cost > new_cost) {
        //::printf("> Literal\n");
        p_ctx[pos+1].arrival_cost = new_cost;
        p_ctx[pos+1].tag = tag;
        p_ctx[pos+1].literal = buf[pos];
        p_ctx[pos+1].length = 1;
        p_ctx[pos+1].offset = offset;
        // move contexts along
        p_ctx[pos+1].offset_ctx = p_ctx[pos].offset_ctx;
        ::memcpy(p_ctx[pos+1].pmr_ctx,p_ctx[pos].pmr_ctx,sizeof(pmr_ctx_t)*PMR_CTX_SIZE);
    }

    return new_cost;
}

/**
 * @brief Calculate the cost for encoding a match at the file @p pos
 *        of some length forward in the @p buf.
 *
 * @param[in] pos     The current absolute position in the input buffer.
 * @param[in] p_match A ptr to an array of offset+length pairs for matches.
 * @param[in] num_matches The number of matches in the @p p_match array.
 * @param[inout] c    A ptr to the array of costs where we calculate
 *                    the arrival cost for each position of the file
 *                    to be compressed.
 * @param[in] buf     A ptr to the input buffer (this is practically the
 *                    whole file to be compressed).
 *
 * @return 0.
 */


int bfhp_cost::impl_match_cost(int pos, cost_base* c, const char* buf)
{
    cost_with_ctx* p_ctx = static_cast<cost_with_ctx*>(c);
    int steps;
    int backward_steps;

    // Unused at the moment..
    (void)buf;

    for (int n = 0; n < p_ctx[pos].num_matches; n++) {
        int offset = p_ctx[pos].matches[n].offset;
        int length = p_ctx[pos].matches[n].length;
        backward_steps = m_backward_steps <= (length-1) ? m_backward_steps : length-1;

        // Sanity checks..
        assert(length > 1);
        assert(offset > 0);
        assert(offset < 16777216);
        assert(length-backward_steps > 1);
        steps = 0;

        do {
            int tag = lz_tag_type::none;
            uint32_t new_cost = p_ctx[pos].arrival_cost + 8;
    
            // Get currect match node offset context
            int offset_ctx = p_ctx[pos].offset_ctx;
            
            // Load current PMR state for this match node..
            m_pmr.state_load(p_ctx[pos].pmr_ctx);
            
            if (length-steps > 15) {
                new_cost += 8;
            }

            // First chcek for PMRs
            int pmr_index = m_pmr.find_pmr_by_offset(offset);

            if (pmr_index >= 0) {
                tag = lz_tag_type::bfhp_pmr0 + pmr_index;
            }

            // If no PMR was found check for osset context
            if (pmr_index < 0) {
                if (offset < 256) {
                    tag = lz_tag_type::bfhp_mml2_off8;
                    new_cost += 8;
                } else if ((offset >> 8) == offset_ctx) {
                    tag = lz_tag_type::bfhp_mml2_off_ctx;
                    offset = 0xff;
                    new_cost += 8;
                } else if (offset < 65536) {
                    tag = lz_tag_type::bfhp_mml2_off16;
                    new_cost += 16;
                    offset_ctx = offset >> 8;
                } else {
                    tag = lz_tag_type::bfhp_mml2_off24;
                    new_cost += 24;
                    offset_ctx = offset >> 8;
                }
            }
            if (p_ctx[pos+length-steps].arrival_cost > new_cost) {
                // Update the tag, offset, length, etc
                p_ctx[pos+length-steps].tag          = tag;
                p_ctx[pos+length-steps].length       = length-steps;
                p_ctx[pos+length-steps].offset       = offset;
                p_ctx[pos+length-steps].arrival_cost = new_cost;
                // update contexts..
                p_ctx[pos+length-steps].offset_ctx = offset_ctx;
#ifdef PMR_UPDATE_WITH_SHORT_OFFSET
                m_pmr.update(pmr_index,offset);
#else
                if (offset >= 256) {
                    m_pmr.update(pmr_index,offset);
                }
#endif
                m_pmr.state_save(p_ctx[pos+length-steps].pmr_ctx);
            }
        } while (steps++ < backward_steps);
    }
    return 0;
}

int bfhp_cost::impl_init_cost(cost_base* c, int sta, int len, bool initial)
{
    cost_with_ctx* p_ctx = static_cast<cost_with_ctx*>(c);
    assert(c != NULL);
    m_pmr.reinit();
    m_pmr.dump();

    // Unused at the moment..
    (void)initial;

    p_ctx[sta].next = 0;
    p_ctx[sta].num_literals = 0;
    p_ctx[sta].arrival_cost = 0;
    p_ctx[sta].offset_ctx = m_offset_ctx;
    m_pmr.state_save(p_ctx[sta].pmr_ctx);
    
    for (int n = sta+1; n < len+1; n++) {
        p_ctx[n].next = 0;
        p_ctx[n].arrival_cost = LZ_MAX_COST;
        p_ctx[n].num_literals = 0;
    }
    return 0;
}

cost_base* bfhp_cost::impl_alloc_cost(int len, int max_chain)
{
    cost_with_ctx* cc;
    match* mm;
    int n;

    cc = new cost_with_ctx[len+1];
    mm = new match[(len+1)*max_chain];

    for (n = 0; n < len+1; n++) {
        cc[n].matches = &mm[n*max_chain];
    }

    return cc;
}


int bfhp_cost::impl_free_cost(cost_base* cost)
{
    cost_with_ctx* cc = static_cast<cost_with_ctx*>(cost);
    delete[] cc->matches;
    delete[] cc;
    return 0;
}

void bfhp_cost::calc_initial_pmr8(cost_with_ctx* p_ctx, int* p0_3, int len)
{   
    int n, m, o, c;
    int offsets_cnt[256];
    int offset;

    for (n = 0; n <256; n++) {
        offsets_cnt[n] = 0;
    }

    n = 1;
    c = 0;

    for (n = 0; n < INTIAL_PMR_DIST && n < len; n++) {
        if (p_ctx[n].num_matches > 0) {
            for (o = 0; o < p_ctx[n].num_matches; o++) {
                offset = p_ctx[n].matches[o].offset ;

                if (offset < 256) {
                    ++offsets_cnt[offset];
                }
            }
        }
    }

    for (n = 0; n < PMR_MAX_NUM; n++) {
        c = 0;
        offset = -1;

        for (m = 0; m < 256; m++) {
            if (offsets_cnt[m] > c) {
                c = offsets_cnt[m];
                offset = m;
            }
        }
        
        if (offset >= 0) {
            p0_3[n] = offset;
            offsets_cnt[offset] = 0;
        }
    }
}



