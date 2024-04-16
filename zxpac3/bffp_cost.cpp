/**
 * @file bffp_cost.cpp
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
#include "bffp_cost.h"

/**
 * @brief The defaulr constructor for the bffp_cost class. 
 * @details tbd
 * 
 * @param[in] p0   PMR0 initial offset value.
 * @param[in] p1   PMR1 initial offset value.
 * @param[in] p2   PMR2 initial offset value.
 * @param[in] p3   PMR3 initial offset value.
 * @param[in] ins  MTF256 new code insert position (must be between 0
 *                 and 128-@p res).
 * @param[in] res  The number of MTF256 reserved code points for escapes.
 * @param[in] mex  The max number of MTF moves. 0 = always to first.
 * @param[in] oc   The initial long offset context. Should be between
 *                 0x01 and 0xffff. The value is the "offset>>8".
 * @param[in] bs   The number of backward steps for better match checking.
 *                 Value 0 means only the 'match_length' of the match is
 *                 considered. Value 1 would mean the 'match_length' and
 *                 'match_length-1' are considered and so on. The values
 *                 must be between 0 and 254. In practise the value should
 *                 as small as possible.
 * @param[in] fwd  True if forward references, false if history references.
 *
 * @return None.
 *
 * @note The constructor may throw exceptions.
 */

bffp_cost::bffp_cost(int bs):
    m_backward_steps(bs)
{
    if (bs < 0 || bs > 256-2) {
        EXCEPTION(std::out_of_range,"Backward steps must be > 0 and < 254");
    }
}

bffp_cost::~bffp_cost()
{
    std::cout << "bffp_cost::~bffp_cost()\n";
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


int bffp_cost::impl_literal_cost(int pos, cost_base* c, const char* buf)
{
    cost_with_ctx* p_ctx = static_cast<cost_with_ctx*>(c);
    int bits = 0;
    uint32_t new_cost = p_ctx[pos].arrival_cost + 8;

#ifdef BFFP_DEBUG
    ::printf("Literal: %d, new_cost %d, arrival_cost at %d = %u\n",
        pos,new_cost,pos+1,p_ctx[pos+1].arrival_cost);
#endif // BFFP_DEBUG
    if (p_ctx[pos+1].arrival_cost > new_cost) {
        p_ctx[pos+1].arrival_cost = new_cost;
        p_ctx[pos+1].tag = lz_tag_type::bffp_lit;
        p_ctx[pos+1].literal = buf[pos];
        p_ctx[pos+1].length = 1;
        p_ctx[pos+1].offset = 0;
#ifdef MTF_CTX_SAVING_ENABLED
#endif // MTF_CTX_SAVING_ENABLED
        bits = 8;
    }

    return bits;
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


int bffp_cost::impl_match_cost(int pos, cost_base* c, const char* buf)
{
    cost_with_ctx* p_ctx = static_cast<cost_with_ctx*>(c);
    int tag;
    uint32_t new_cost;
    int steps;
    int backward_steps;

    // current node arrival cost for short of long match lengths

    for (int n = 0; n < p_ctx[pos].num_matches; n++) {
        int offset = p_ctx[pos].matches[n].offset;
        int length = p_ctx[pos].matches[n].length;
        backward_steps = m_backward_steps <= (length-1) ? m_backward_steps : length-1;

        // Sanity checks..
        assert(length > 1);
        assert(offset > 0);
        assert(offset < 16777216);
        assert(length-backward_steps > 1);
        
        // Step #2: check for other offset
        steps = 0;

        do {
            new_cost = p_ctx[pos].arrival_cost + 8;

            if (length-steps > 15) {
                new_cost += 8;
            }
            if (offset < 256) {
                tag = lz_tag_type::bffp_mml2_off8;
                new_cost += 8;
            } else if (offset < 65536) {
                tag = lz_tag_type::bffp_mml2_off16;
                new_cost += 16;
            } else {
                tag = lz_tag_type::bffp_mml2_off24;
                new_cost += 24;
            }
#ifdef BFFP_DEBUG   
            ::printf("Match: %d, %d:%d, tag %d, new cost %d, arrival cost %d\n",
                pos,offset,length-steps,tag,new_cost,p_ctx[pos+length-steps].arrival_cost);
#endif // BFFP_DEBUG
            if (p_ctx[pos+length-steps].arrival_cost > new_cost) {
#ifdef BFFP_DEBUG
                ::printf("Match cost was %d\n",new_cost-p_ctx[pos].arrival_cost);
#endif // BFFP_DEBUG
                // Update the tag, offset, length, etc
                p_ctx[pos+length-steps].tag          = tag;
                p_ctx[pos+length-steps].length       = length-steps;
                p_ctx[pos+length-steps].offset       = offset;
                p_ctx[pos+length-steps].arrival_cost = new_cost;
            }
        } while (steps++ < backward_steps);
    }
    return 0;
}

int bffp_cost::impl_init_cost(cost_base* c, int sta, int len, bool initial)
{
    cost_with_ctx* p_ctx = static_cast<cost_with_ctx*>(c);
    assert(c != NULL);
  
    ::printf("In bffp_cost::impl_init_cost(%p,%d,%d)\n",c,sta,len);
    p_ctx[sta].next = 0;
    p_ctx[sta].arrival_cost = 0;
    p_ctx[sta].num_literals = 0;
    
    if (initial) {
        p_ctx[sta].tag = lz_tag_type::none;
    }

    for (int n = sta+1; n < len+1; n++) {
        p_ctx[n].next = 0;
        p_ctx[n].arrival_cost = LZ_MAX_COST;
        p_ctx[n].num_literals = 0;
        
        if (initial) {
            p_ctx[n].tag = lz_tag_type::none;
        }
    }
    return 0;
}

cost_base* bffp_cost::impl_alloc_cost(int len, int max_chain)
{
    cost_with_ctx* cc;
    match* mm;
    int n;

    ::printf("In bffp_cost::impl_alloc_cost(len=%d,max_chain=%d)\n",
        len,max_chain);

    cc = new cost_with_ctx[len+1];
    mm = new match[(len+1)*max_chain];

    for (n = 0; n < len+1; n++) {
        ::printf("Indexing matches %d:%d\n",n,n*max_chain);
        cc[n].matches = &mm[n*max_chain];
    }

    return cc;
}


int bffp_cost::impl_free_cost(cost_base* cost)
{
    cost_with_ctx* cc = static_cast<cost_with_ctx*>(cost);
    delete[] cc->matches;
    delete[] cc;
    return 0;
}

