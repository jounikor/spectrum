/**
 * @file cost4c.cpp
 * @brief Calculates an approximate cost for the LZ encoding.
 * @version 0.1
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 1-Aug-2025
 * @copyright The Unlicense
 *
 * @note TODOs - the current design still has few (pun intended) flaws I want to change:
 *   - For the literal/match/PMR selection prefix codes are still shared between
 *     the cost class and the encoder class. They should all be within the
 *     cost class -> requires 'some' refactoring and interface changes.
 */

#include <iostream>
#include <iomanip>
#include <cassert>
#include <stdint.h>
#include <string.h>
#include "cost4c.h"
#include "zxpac4c.h"

/**
 * @brief The default constructor for the cost class. 
 * @details tbd
 * 
 * @param[in] p_cfg A ptr to generic LZ configuration container.
 * @param[in] ins   MTF256 new code insert position (must be between 0
 *                  and 128-@p res).
 * @param[in] max   The max number of MTF moves. 0 = always to first.
 *
 * @return None.
 *
 * @note The constructor may throw exceptions.
 */

zxpac4c_cost::zxpac4c_cost(
    const lz_config* p_cfg, int ins, int max): 
    lz_cost(p_cfg)
{
    (void)ins;
    (void)max;

    if (p_cfg->backward_steps < 0 || p_cfg->backward_steps > 256-2) {
        EXCEPTION(std::out_of_range,"Backward steps must be > 0 and < 256");
    }

    m_debug = false;
    m_verbose = false;
}

zxpac4c_cost::~zxpac4c_cost(void)
{
}

int zxpac4c_cost::impl_get_offset_tag(int offset, char& byte_tag, int& bit_tag)
{
    int bits = 8;

    // Must hold: 0 < offset < 131072
    assert(offset < (1 << 17));
    assert(offset > 0);

    while (offset >= (1 << bits)) {
        ++bits;
    }

    return bits;
}

int zxpac4c_cost::impl_get_length_tag(int length, int& bit_tag)
{
    assert(length > 0);
    assert(length < 256);

    int bits = 1;

    while (length >= (1 << bits)) {
        ++bits;
    }

    return bits;
}

int zxpac4c_cost::impl_get_literal_tag(const char* literals, int length, char& byte_tag, int& bit_tag)
{
    // This API sucks quite hard.. but lets try to abuse it anyway.



    (void)literals;
    (void)byte_tag;
    assert(length < 256);
    return impl_get_length_tag(length,bit_tag);
}

int zxpac4c_cost::impl_get_offset_bits(int offset)
{
    assert(offset < 131072);
    
    if (offset == 0) {
        return 0;
    }
    if (offset < 128) {
        return (7+1 + 0);
    } else if (offset < 256) {
        return (7+3+0);
    } else if (offset < 512) {
        return (7+3+1);
    } else if (offset < 1024) {
        return (7+4+2);
    } else if (offset < 2048) {
        return (7+4+3);
    } else if (offset < 4096) {
        return (7+5+4);
    } else if (offset < 8192) {
        return (7+5+5);
    } else if (offset < 16384) {
        return (7+6+6);
    } else if (offset < 32768) {
        return (7+6+7);
    } else if (offset < 65536) {         // up to 65535
        return (7+6+8);
    } else if (offset < 131072) {
        return (7+6+9);
    }
    return 0;
}

int zxpac4c_cost::impl_get_length_bits(int length)
{
    assert(length > 0);
    assert(length < 256);

    // matchlen cost..
    if (length == 1) {
        return (1+0);
    } else if (length < 4) {
        return (2+1);
    } else if (length < 8) {
        return (3+2);
    } else if (length < 16) {
        return (4+3);
    } else if (length < 32) {
        return (5+4);
    } else if (length < 64) {
        return (6+5);
    } else if (length < 128) {
        return (7+6);
    }
    // < 256
    return (7+7);
}

int zxpac4c_cost::impl_get_literal_bits(char literal, bool is_ascii)
{
    (void)literal;
    (void)is_ascii;
    return 8;
}

/**
 * @brief Calculate the cost for encoding a literal character at the
 *        file @p pos in the @p buf. Also update literals tANS frequencies.
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


int zxpac4c_cost::impl_literal_cost(int pos, cost* c, const char* buf)
{
    cost* p_ctx = &c[pos];
    uint32_t new_cost = p_ctx->arrival_cost;
    int offset = p_ctx->offset;
    int num_literals = p_ctx->num_literals;

    if (pos >= p_ctx->pmr_offset && (buf[pos-p_ctx->pmr_offset] == buf[pos])) {
        // PMR of length 1 
        offset = p_ctx->pmr_offset;
        num_literals = 1;
    } else {
        // literal run
        if (num_literals > 0) {
            new_cost -= impl_get_length_bits(num_literals);
        }
        ++num_literals;
        new_cost += 8;
        offset = 0;
    }
    if (lz_get_config()->is_ascii == false || p_ctx->last_was_literal == false) {
        // tag of 1bit
        new_cost = new_cost + 1;
    }
    
    new_cost += impl_get_length_bits(num_literals);

    if (p_ctx[1].arrival_cost >= new_cost) {
        p_ctx[1].arrival_cost = new_cost;
        p_ctx[1].length = 1;
        p_ctx[1].offset = offset;
        p_ctx[1].pmr_offset = p_ctx->pmr_offset;

        if (offset == 0) {
            p_ctx[1].last_was_literal = true;
            p_ctx[1].num_literals = num_literals;
        } else {
            p_ctx[1].last_was_literal = false;    
            p_ctx[1].num_literals = 0;
        }
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


int zxpac4c_cost::impl_match_cost(int pos, cost* c, const char* buf, int offset, int length)
{
    cost* p_ctx = &c[pos];
    bool pmr_found = false;
    int pmr_offset; 
    uint32_t new_cost;
    int encode_length;
    int tag_cost;
    int n;
    
    if (lz_get_config()->is_ascii == false || p_ctx->last_was_literal == false) {
        tag_cost = 1;
    } else {
        tag_cost = 0;
    }

    pmr_offset = p_ctx->pmr_offset; 
    new_cost = p_ctx->arrival_cost + tag_cost;
        
    if (offset == pmr_offset) {
        offset = 0;
        encode_length = length;
        pmr_found = true;
    } else {
        pmr_offset = offset;
        encode_length = length - 1;
    }
       
    assert(encode_length > 0);
    assert(offset < 131072);
    new_cost += get_offset_bits(offset);
    new_cost += get_length_bits(encode_length);
           
    if (p_ctx[length].arrival_cost >= new_cost) {
        p_ctx[length].offset       = offset;
        p_ctx[length].pmr_offset   = pmr_offset;
        p_ctx[length].arrival_cost = new_cost;
        p_ctx[length].length       = length;
        p_ctx[length].last_was_literal = false;
        p_ctx[length].num_literals = 0;
    }
    
    pmr_offset = p_ctx->pmr_offset;

    if (pmr_found == false && pos >= pmr_offset) {
        int max_match = lz_get_config()->max_match;
        
        n = pos < m_max_len - max_match ? max_match : m_max_len - pos;
        length = check_match(&buf[pos],&buf[pos-pmr_offset],n);
        assert(length <= max_match);

        if (length >= lz_get_config()->min_match) {
            new_cost = p_ctx->arrival_cost + tag_cost;
            new_cost += get_length_bits(length);
            
            if (p_ctx[length].arrival_cost >= new_cost) {
                p_ctx[length].offset       = 0;
                p_ctx[length].pmr_offset   = pmr_offset;
                p_ctx[length].arrival_cost = new_cost;
                p_ctx[length].length       = length;
                p_ctx[length].last_was_literal = false;
                p_ctx[length].num_literals = 0;
    }   }   }
    
    return length >= lz_get_config()->good_match ? lz_get_config()->good_match : 1;
}


/**
 * @brief Initialize cost and tANS..
 *
 *
 *
 */
int zxpac4c_cost::impl_init_cost(cost* p_ctx, int sta, int len, int pmr)
{
    int n;
    assert(p_ctx != NULL);

    // Initialize the cost array
    p_ctx[sta].next         = 0;
    p_ctx[sta].num_literals = 0;
    p_ctx[sta].arrival_cost = 0;
    p_ctx[sta].pmr_offset   = pmr;
    p_ctx[sta].pmr_length   = 0;
    p_ctx[sta].last_was_literal = false;

    for (n = sta+1; n < len+1; n++) {
        p_ctx[n].arrival_cost = LZ_MAX_COST;
        p_ctx[n].num_literals = 0;
    }
    
    // Initialize tANS symbol frequencies
    for (n = 0; n < TANS_NUM_LITERAL_SYM; n++) {
        m_literal_sym_freq[n] = 0;
    }
    for (n = 0; n < TANS_NUM_MATCH_SYM; n++) {
        m_match_sym_freq[n] = 0;
    }
    for (n = 0; n < TANS_NUM_OFFSET_SYM; n++) {
        m_offset_sym_freq[n] = 0;
    }

    return 0;
}

cost* zxpac4c_cost::impl_alloc_cost(int len, int max_chain)
{
    cost* cc;
    (void)max_chain;

    m_max_len = len;
    
    try {
        cc = new cost[len+1];
    } catch (...) {
        throw;
    }

    return cc;
}


int zxpac4c_cost::impl_free_cost(cost* cost)
{
    if (cost) {
        delete[] cost;
    }
    return 0;
}
