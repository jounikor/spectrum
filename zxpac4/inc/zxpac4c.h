/**
 * @file zxpac4c.h
 * @brief ZX Pac v4c with 32K window class definitions
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.14
 * @date summer 2025
 * @copyright The Unlicense
 *
 *
 */


#ifndef _ZXPAC4C_H_INCLUDED
#define _ZXPAC4C_H_INCLUDED

#include <vector>
#include <cstdint>
#include "lz_base.h"
#include "lz_util.h"
#include "hash.h"
#include "cost4c.h"

/**
 The binary encoding of zxpac4c format:
@verbatim

  Literal after a match:
  0 + literal run length + (literal bytes)

  PMR after a literal
  0 + matchlen

  Match:
  1 + matchlen + offset_high_bits + lower_8_bits_as_a_byte

 matchlen tANS symbols from 0 to 8
  0 + [0] = 1                 // for PMR literal
  1 + [1] = 2 -> 3            // n
  2 + [2] = 4 -> 7            // nn
  3 + [3] = 8 -> 15           // nnn
  4 + [4] = 16 -> 31          // nnnn
  5 + [5] = 32 -> 64          // nnnnn
  6 + [6] = 64 -> 127         // nnnnnn
  7 + [7] = 128 -> 255        // nnnnnnn
  8 + [8] = 256 -> 511
  9 + [9] = 512 -> 1023


 literal run len tANS symbols from 0 to 8
  0 + [0] = 1                 // 
  1 + [1] = 2 -> 3            // n
  2 + [2] = 4 -> 7            // nn
  3 + [3] = 8 -> 15           // nnn
  4 + [4] = 16 -> 31          // nnnn
  5 + [5] = 32 -> 64          // nnnnn
  6 + [6] = 64 -> 127         // nnnnnn
  7 + [7] = 128 -> 255        // nnnnnnn
  8 + [8] = 256 -> 511
  9 + [9] = 512 -> 1023
  
 offset tANS symbols from 0 to 8
  
  0 + [0] =     1 -> 255      // <- encoded as a byte
  1 + [1] =   256 -> 511      // n
  2 + [2] =   512 -> 1023     // nn
  3 + [3] =  1024 -> 2047     // nnn
  4 + [4] =  2048 -> 4095     // nnnn
  5 + [5] =  4096 -> 8191     // nnnnn
  6 + [6] =  8192 -> 16383    // nnnnnn
  7 + [7] = 16384 -> 32767    // nnnnnnn
  8 + [8] = 32768 -> 65535    // nnnnnnnn
  9 + [9] = 65536 -> 131071   // nnnnnnnnn


@endverbatim

*/

#define ZXPAC4C_INIT_PMR_OFFSET				5 
#define ZXPAC4C_MATCH_MIN					2
#define ZXPAC4C_MATCH_MAX					1023
#define ZXPAC4C_MATCH_GOOD					63
#define ZXPAC4C_OFFSET_MATCH2_THRESHOLD		512
#define ZXPAC4C_OFFSET_MATCH3_THRESHOLD		4096
#define ZXPAC4C_WINDOW_MAX					131072
#define ZXPAC4C_OFFSET_MIN					256
#define ZXPAC4C_HEADER_SIZE					4
#define ZXPAC4C_LITRUN_MAX					1023

/**
 * @class matches utils.h
 * @brief A container class to hold all matches within the sliding window.
 * 
 *
 */

class zxpac4c : public lz_base {
    hash3 m_lz;
    cost* m_cost_array;
    match* m_match_array;
    int m_alloc_len;
    zxpac4c_cost m_cost;
private:
    int encode_history(const char* buf, char* out, int len, int pos);
public:
    zxpac4c(const lz_config* cfg, int ins=-1, int max=-1);
    ~zxpac4c();
    int lz_search_matches(char* buf, int len, int interval);
    int lz_parse(const char* buf, int len, int interval);
    int lz_encode(char* buf, int len, char* outb, std::ofstream* ofs);

    const cost* lz_get_result(void) { return m_cost_array; }
    const cost* lz_cost_array_get(int len);
    void lz_cost_array_done(void);
    bool is_ascii(void) { return m_lz_config->is_ascii; }
    bool only_better(void) { return m_lz_config->only_better_matches; }

	void preload_tans(int type, uint8_t* freqs=NULL, int len=0);
};


#endif  // _ZXPAC4C_H_INCLUDED
