/**
 * @file cost4b.cpp
 * @brief Calculates an approximate cost for the LZ encoding.
 * @version 0.3
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 7-Apr-2024
 * @date 4-Aug-2024
 * @date 3-Feb-2025
 * @copyright The Unlicense
 *
 * @note TODOs - the current design still has few (pun intended) flaws I want to change:
 *   - For the literal/match/PMR selection prefix codes are still shared between
 *     the cost class and the encoder class. They should all be within the
 *     cost class -> requires 'some' refactoring and interface changes.
 *   - Allow two maximum window sizes: 32K and 128K. This would allow shorter emcoding
 *     of offset prefixes when the source file is e.g. never longer than 32K or so.
 */

#include <iostream>
#include <iomanip>
#include <cassert>
#include <stdint.h>
#include <string.h>
#include "cost4b.h"
#include "zxpac4b.h"

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

zxpac4b_cost::zxpac4b_cost(
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

zxpac4b_cost::~zxpac4b_cost(void)
{
}

int zxpac4b_cost::impl_get_offset_tag(int offset, char& byte_tag, int& bit_tag)
{
    if (offset < 128) {
        // (0ooooooo)
        byte_tag = offset;
        bit_tag = 0;
        return 0;
    } else if (offset < 256) {
        // (1ooooooo) + 00
        byte_tag = offset;
        bit_tag = 0;
        return 2;
    } else if (offset < 512) {
        // (1ooooooo) + 01 + o
        byte_tag = offset >> 1;
        bit_tag = (0x1 << 1) | (offset & 0x01);
        return 3;
    } else if (offset < 1024) {
        // (1ooooooo) + 100 + oo
        byte_tag = offset >> 2;
        bit_tag = (0x4 << 2) | (offset & 0x03);
        return 5;
    } else if (offset < 2048) {
        // (1ooooooo) + 101 + ooo
        byte_tag = offset >> 3;
        bit_tag = (0x5 << 3) | (offset & 0x07);
        return 6;
    } else if (offset < 4096) {
        // (1ooooooo) + 1100 + oooo
        byte_tag = offset >> 4;
        bit_tag = (0xc << 4) | (offset & 0x0f);
        return 8;
    } else if (offset < 8192) {
        // (1ooooooo) + 1101 + oooo + o
        byte_tag = offset >> 5;
        bit_tag = (0xd << 5) | (offset & 0x1f);
        return 9;
    } else if (offset < 16384) {
        // (1ooooooo) + 11100 + ooo + ooo
        byte_tag = offset >> 6;
        bit_tag = (0x1c << 6) | (offset & 0x3f);
        return 11;
    } else if (offset < 32768) {
        // (1ooooooo) + 11101 + ooo + oooo
        byte_tag = offset >> 7;
        bit_tag = (0x1d << 7) | (offset & 0x7f);
        return 12;
    } else if (offset < 65536) {
        // (1ooooooo) + 11110 + ooo + ooooo
        byte_tag = offset >> 8;
        bit_tag = (0x1e << 8) | (offset & 0xff);
        return 13;
    } else /*(offset < 131072)*/ {
    // (1ooooooo) + 11111 + ooo + oooooo
        byte_tag = offset >> 9;
        bit_tag = (0x1f << 9) | (offset & 0x1ff);
        return 14;
    }
}

int zxpac4b_cost::impl_get_length_tag(int length, int& bit_tag)
{
    assert(length > 0);
    assert(length < 256);

    int mask_bit = 7;
    int bits = 1;
    int mask;
    bit_tag = 0;

    while ((1 << mask_bit) > length) {
        mask_bit--;
    }

    if (mask_bit == 0) {
        // 0 + [0] = 1
        return bits;
    }

    mask = 1 << (mask_bit - 1);
   
    while (mask > 0) {
        bit_tag = (bit_tag | 0x1) << 2;
        bit_tag = mask & length ? bit_tag | 0x2 : bit_tag;
        bits += 2;
        mask >>= 1;
    }

    if (mask_bit == 7) {
        bit_tag >>= 1;
        bits -= 1;
    }

    return bits;
}

int zxpac4b_cost::impl_get_literal_tag(const char* literals, int length, char& byte_tag, int& bit_tag)
{
    (void)literals;
    (void)byte_tag;
    assert(length < 256);
    return impl_get_length_tag(length,bit_tag);
}

int zxpac4b_cost::impl_get_offset_bits(int offset)
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

int zxpac4b_cost::impl_get_length_bits(int length)
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

int zxpac4b_cost::impl_get_literal_bits(char literal, bool is_ascii)
{
    (void)literal;
    (void)is_ascii;
    return 8;
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


int zxpac4b_cost::impl_literal_cost(int pos, cost* c, const char* buf)
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


int zxpac4b_cost::impl_match_cost(int pos, cost* c, const char* buf, int offset, int length)
{
    cost* p_ctx = &c[pos];
    bool pmr_found = false;
    int local_pmr_offset; 
    uint32_t new_cost;
    int encode_length;
    int tag_cost;
    int n;
    
    if (lz_get_config()->is_ascii == false || p_ctx->last_was_literal == false) {
        tag_cost = 1;
    } else {
        tag_cost = 0;
    }

    local_pmr_offset = p_ctx->pmr_offset; 
    new_cost = p_ctx->arrival_cost + tag_cost;
        
    if (offset == local_pmr_offset) {
        offset = 0;
        encode_length = length;
        pmr_found = true;
    } else {
        local_pmr_offset = offset;
        encode_length = length - 1;
    }
       
    assert(encode_length > 0);
    assert(offset < 131072);
    new_cost += get_offset_bits(offset);
    new_cost += get_length_bits(encode_length);
           
    if (p_ctx[length].arrival_cost >= new_cost) {
        p_ctx[length].offset       = offset;
        p_ctx[length].pmr_offset   = local_pmr_offset;
        p_ctx[length].arrival_cost = new_cost;
        p_ctx[length].length       = length;
        p_ctx[length].last_was_literal = false;
        p_ctx[length].num_literals = 0;
    }
    
    local_pmr_offset = p_ctx->pmr_offset;

    if (pmr_found == false && pos >= local_pmr_offset) {
        int max_match = lz_get_config()->max_match;
        
        n = pos < m_max_len - max_match ? max_match : m_max_len - pos;
        length = check_match(&buf[pos],&buf[pos-local_pmr_offset],n);
        assert(length <= max_match);

        if (length >= lz_get_config()->min_match) {
            new_cost = p_ctx->arrival_cost + tag_cost;
            new_cost += get_length_bits(length);
            
            if (p_ctx[length].arrival_cost >= new_cost) {
                p_ctx[length].offset       = 0;
                p_ctx[length].pmr_offset   = local_pmr_offset;
                p_ctx[length].arrival_cost = new_cost;
                p_ctx[length].length       = length;
                p_ctx[length].last_was_literal = false;
                p_ctx[length].num_literals = 0;
    }   }   }
    
    return length >= lz_get_config()->good_match ? lz_get_config()->good_match : 1;
}

int zxpac4b_cost::impl_init_cost(cost* p_ctx, int sta, int len, int pmr)
{
    assert(p_ctx != NULL);

    p_ctx[sta].next         = 0;
    p_ctx[sta].num_literals = 0;
    p_ctx[sta].arrival_cost = 0;
    p_ctx[sta].pmr_offset   = pmr;
    p_ctx[sta].last_was_literal = false;

    for (int n = sta+1; n < len+1; n++) {
        p_ctx[n].arrival_cost = LZ_MAX_COST;
        p_ctx[n].num_literals = 0;
    }
    return 0;
}

cost* zxpac4b_cost::impl_alloc_cost(int len, int max_chain)
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


int zxpac4b_cost::impl_free_cost(cost* cost)
{
    if (cost) {
        delete[] cost;
    }
    return 0;
}
