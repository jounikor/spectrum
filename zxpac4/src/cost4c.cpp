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
#include <cmath>
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
}

zxpac4c_cost::~zxpac4c_cost(void)
{
}

/**
 *
 *
 * @return Number of bits needed for the TAG + low part of the offset.
 *         \p hi_byte up to min_offset bit of the bit part of the offset.
 *         \p bit_tag contains both TAG + low part of the offset.
 *
 */

int zxpac4c_cost::impl_get_offset_tag(int offset, char& literal, int& bit_tag)
{
    uint32_t b;
    uint8_t k;
    int len_bits;
	int sym;
	int encode_offset;
	int min_offset_bits = log2(m_lz_config->min_offset);

    // Must hold: 0 < offset < ZXPAC4C_WINDOW_MAX
    assert(offset < m_lz_config->window_size);
    assert(offset > 0);
	assert(min_offset_bits <= 8);

	len_bits = impl_get_offset_bits(offset);
	
	if (offset < m_lz_config->min_offset) {
		sym = 0;
		len_bits = 0;
		encode_offset = 0;
	} else {
		len_bits = len_bits - min_offset_bits;
		sym = len_bits + 1;
		encode_offset = offset >> min_offset_bits;
		encode_offset = encode_offset & ((1 << len_bits) - 1);
	}

	literal = offset & ((1 << min_offset_bits) - 1);
	m_tans_offset.encode(sym,k,b);

    if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
        std::cerr << "tANS offset: 0x" << std::hex << std::setfill('0') << std::setw(2)
				  << std::right << static_cast<uint32_t>(literal & 0xff)
				  << ", " << min_offset_bits << ", " 
				  << std::dec << std::setw(6) << std::setfill(' ') << encode_offset
				  << " (" << std::setw(6) << offset << "), "
				  << std::setw(2) << len_bits
                  << ", k: " << static_cast<int>(k) << ", b: 0x"
				  <<std::setw(2) << std::right << std::hex << std::setfill('0') << b
				  << std::dec;
    }

    bit_tag = b << len_bits;
    bit_tag = bit_tag | encode_offset;

    // tANS + length encoding bits
    return len_bits + k;

}

int zxpac4c_cost::impl_get_length_tag(int length, int& bit_tag)
{
    uint32_t b;
    uint8_t k;
    int len_bits;
    int encode_length = length > m_lz_config->max_literal_run ?  m_lz_config->max_literal_run : length;
    
    assert(length > 0);
    assert(length <= m_lz_config->max_match);
    
	len_bits = impl_get_length_bits(encode_length);
    m_tans_match.encode(len_bits,k,b);

    if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
        std::cerr << std::dec;
        std::cerr << "tANS length: "
				  << std::right << std::dec << std::setw(3) << std::setfill(' ')
                  << encode_length << " (" << std::setw(3) << length << "), " << len_bits
                  << ", k: " << static_cast<int>(k) << ", b: 0x"
				  << std::setw(2) << std::right << std::hex << std::setfill('0') << b
				  << std::dec;
    }

    bit_tag = b << len_bits;
    bit_tag = bit_tag | (encode_length & ((1 << len_bits) - 1));

    // tANS + length encoding bits
    return len_bits + k;
}

int zxpac4c_cost::impl_get_literal_tag(const char* literals, int length, char& byte_tag, int& bit_tag)
{
    // This API sucks quite hard.. but lets try to abuse it anyway.
    (void)literals;
    (void)byte_tag;

    assert(length > 0);
    
    // get the tANS stuff..
    uint32_t b;
    uint8_t k;
    int len_bits;
    int encode_length = length > m_lz_config->max_literal_run ?  m_lz_config->max_literal_run : length;

    len_bits = impl_get_length_bits(encode_length);
    m_tans_literal.encode(len_bits,k,b);

    if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
        std::cerr << std::dec;
        std::cerr << "tANS literal: " << std::left << std::dec
                  << encode_length << " (" << length << ") -> " << len_bits
                  << ", k: " << static_cast<int>(k) << ", b: 0x" 
				  << std::setw(2) << std::right << std::hex << std::setfill('0') << b
				  << std::dec;
    }

    // tag is 0
    bit_tag = b << len_bits;
    bit_tag = bit_tag | (encode_length & ((1 << len_bits) - 1));

    // tANS + length encoding bits
    return len_bits + k;
}

/**
 *
 *
 * @return Number of bits needed to encode the offset in full;
 *         Using (1 << num_bits) | get_bits(num_bits) formula;
 */

int zxpac4c_cost::impl_get_offset_bits(int offset)
{
    assert(offset < m_lz_config->window_size);
    int bits = 0;

    while (offset >= (2 << bits)) {
        ++bits;
    }

    return bits;
}

int zxpac4c_cost::impl_get_length_bits(int length)
{
    int bits = 0;
    while (length >= (2 << bits)) {
        ++bits;
    }
    
    return bits;
}

int zxpac4c_cost::impl_get_literal_bits(char literal, bool is_ascii)
{
    (void)literal;
    (void)is_ascii;
    // somewhat a straight shortcut..
    return 7;
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
    uint32_t new_cost = p_ctx->arrival_cost + 1;
    int offset = p_ctx->offset;
    int num_literals = p_ctx->num_literals;

    if (pos >= p_ctx->pmr_offset && (buf[pos - p_ctx->pmr_offset] == buf[pos])) {
        // PMR of length 1 
        offset = p_ctx->pmr_offset;
        num_literals = 1;
		new_cost += 4;
    } else {
        // get the cost of the new literal
        new_cost += impl_get_literal_bits(buf[pos],false);
        if (num_literals > 0) {
            // substract the previous literal run encoding.. since this is delta..
            new_cost -= impl_get_length_bits(num_literals);
        }
        ++num_literals;

        // mark as a literal run instead of a PMR with length 1
        offset = 0;
    }

    // get the cost of literal run encoding
    new_cost += impl_get_length_bits(num_literals);
    new_cost += predict_tans_cost(TANS_LITERAL_RUN_SYMS,num_literals);

	if (p_ctx[1].arrival_cost >= new_cost) {
        p_ctx[1].arrival_cost = new_cost;
        p_ctx[1].length = 1;
        p_ctx[1].offset = offset;
        p_ctx[1].pmr_offset = p_ctx->pmr_offset;

        if (offset == 0) {
            p_ctx[1].num_literals = num_literals;
        } else {
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
    int pmr_offset; 
    uint32_t new_cost;

    (void)buf;

    // Do we have a bug?
    assert(offset < m_lz_config->window_size);
    assert(length <= m_lz_config->max_match);
    assert(length > 0);

    // Tag cost is 1 bit as a baseline..
    pmr_offset = p_ctx->pmr_offset; 
    new_cost = p_ctx->arrival_cost + 1;
    
    if (pos >= pmr_offset && offset == pmr_offset) {
        // We have a PMR match
		offset = 0;
    } else {
		// Just a normal match. Update the PMR offset
        pmr_offset = offset;
    }

    // weight of offset encoding 
	if (offset > 0) {
		new_cost += get_offset_bits(offset);
		new_cost += predict_tans_cost(TANS_OFFSET_SYMS,offset); 
	}
	// weight of length encoding 
    new_cost += get_length_bits(length);
    new_cost += predict_tans_cost(TANS_LENGTH_SYMS,length);

    if (p_ctx[length].arrival_cost > new_cost) {
        p_ctx[length].offset       = offset;
        p_ctx[length].pmr_offset   = pmr_offset;
        p_ctx[length].arrival_cost = new_cost;
        p_ctx[length].length       = length;
        p_ctx[length].num_literals = 0;
    }
    
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

    for (n = sta+1; n < len+1; n++) {
        p_ctx[n].arrival_cost = LZ_MAX_COST;
        p_ctx[n].num_literals = 0;
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

// New tANS helper functions

int zxpac4c_cost::predict_tans_cost(int type, int value)
{
    (void)value;

    // For now just static costs.. subject to change..

    switch (type) {
    case TANS_LITERAL_RUN_SYMS:
        return TANS_LITERAL_RUN_COST; 
    case TANS_LENGTH_SYMS:
        return TANS_LENGTH_COST;
    case TANS_OFFSET_SYMS:
        return TANS_OFFSET_COST;
    default:
        assert(0);
    }

}

ans_state_t zxpac4c_cost::get_tans_state(int type)
{ 
    switch (type) {
    case TANS_LITERAL_RUN_SYMS:
        return m_tans_literal.get_state(); 
    case TANS_LENGTH_SYMS:
        return m_tans_match.get_state();
    case TANS_OFFSET_SYMS:
        return m_tans_offset.get_state();
    default:
        return -1;
    }
}

int zxpac4c_cost::inc_tans_symbol_freq(int type, uint8_t symbol)
{
    switch (type) {
    case TANS_LITERAL_RUN_SYMS:
        return ++m_literal_sym_freq[symbol];
    case TANS_LENGTH_SYMS:
        return ++m_match_sym_freq[symbol];
    case TANS_OFFSET_SYMS:
        return ++m_offset_sym_freq[symbol];
    default:
        assert(NULL == "Unknown tANS type");
        return -1;
    }
}

void zxpac4c_cost::set_tans_symbol_freqs(int type, uint8_t* freqs, int len)
{
    int* local_freq;
    int local_len;

    switch (type) {
    case TANS_LITERAL_RUN_SYMS:
        local_freq = m_literal_sym_freq;
        local_len = sizeof(m_literal_sym_freq);
        break;
    case TANS_LENGTH_SYMS:
        local_freq = m_match_sym_freq;
        local_len = sizeof(m_match_sym_freq);
        break;
    case TANS_OFFSET_SYMS:
        local_freq = m_offset_sym_freq;
        local_len = sizeof(m_offset_sym_freq);
        break;
    default:
        assert(NULL == "Unknown tANS type");
    }

    if (freqs == NULL) {
        ::memset(local_freq,0,local_len);
    } else {
        assert(local_len == len);
        ::memcpy(local_freq,freqs,len);
    }
}



void zxpac4c_cost::build_tans_tables(void)
{
    m_tans_literal.init_tans(m_literal_sym_freq,TANS_NUM_LITERAL_SYM);
    m_tans_match.init_tans(m_match_sym_freq,TANS_NUM_MATCH_SYM);
    m_tans_offset.init_tans(m_offset_sym_freq,TANS_NUM_OFFSET_SYM);
}

int zxpac4c_cost::get_tans_scaled_symbol_freqs(int type, uint8_t* freqs, int len)
{
    int n,m;
    const int* p;

    switch (type) {
    case TANS_LITERAL_RUN_SYMS:
        assert(len >= m_tans_literal.get_Ls_len());
        m = m_tans_literal.get_Ls_len();
        p = m_tans_literal.get_scaled_Ls();
        break;
    case TANS_LENGTH_SYMS:
        assert(len >= m_tans_match.get_Ls_len());
        m = m_tans_match.get_Ls_len();
        p = m_tans_match.get_scaled_Ls();
        break;
    case TANS_OFFSET_SYMS:
        assert(len >= m_tans_offset.get_Ls_len());
        m = m_tans_offset.get_Ls_len();
        p = m_tans_offset.get_scaled_Ls();
        break;
    default:
        return -1;
    }

    for (n = 0; n < m; n++) {
        freqs[n] = p[n];
    }

    return m;
}

void zxpac4c_cost::dump(int type)
{
    switch (type) {
    case TANS_LITERAL_RUN_SYMS:
        std::cerr << "** TANS_LITERAL_RUN_SYMS ";
        m_tans_literal.dump();
        break;
    case TANS_LENGTH_SYMS:
        std::cerr << "** TANS_LENGTH_SYMS -----";
        m_tans_match.dump();
        break;
    case TANS_OFFSET_SYMS:
        std::cerr << "** TANS_OFFSET_SYMS -----";
        m_tans_offset.dump();
        break;
    default:
        assert(0);
    }

}
