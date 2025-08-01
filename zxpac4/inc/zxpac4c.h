/**
 * @file zxpac4c.h
 * @brief ZX Pac v4c with 32K window class definitions
 * @author Jouni 'Mr.SpivKorhonen
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
  
  PMR i.e. a Literal after a literal:
  0 + matchlen   -> PMR, where offset repeats

  Match:
  1 + (nnnnnnnn) + reminder of offset + matchlen
  1 + (00000000) -> PMR where offset and matchlen repeat

 matchlen tANS symbols from 0 to 8
  0 + [0] = 1                 // For copying a single literal with PMR
  1 + [1] = 2 -> 3            // n
  2 + [2] = 4 -> 7            // nn
  3 + [3] = 8 -> 15           // nnn
  4 + [4] = 16 -> 31          // nnnn
  5 + [5] = 32 -> 64          // nnnnn
  6 + [6] = 64 -> 127         // nnnnnn
  7 + [7] = 128 -> 255        // nnnnnnn

  to be tested later..
  8 + [8] = 256 -> 511        // nnnnnnnn
  9 + [9] = 512 -> 1023       // nnnnnnnnn

 literal run len tANS symbols from 0 to 8
  0 + [0] = 1                 // 
  1 + [1] = 2 -> 3            // n
  2 + [2] = 4 -> 7            // nn
  3 + [3] = 8 -> 15           // nnn
  4 + [4] = 16 -> 31          // nnnn
  5 + [5] = 32 -> 64          // nnnnn
  6 + [6] = 64 -> 127         // nnnnnn
  7 + [7] = 128 -> 255        // nnnnnnn
  
to be tested later
  8 + [8] = 256 -> 511        // nnnnnnnn
  9 + [9] = 512 -> 1023       // nnnnnnnnn
 
 offset tANS symbols from 0 to 8
  
  0 + [0] =     1 -> 255      // <- always encoded as a byte 
  1 + [1] =   256 -> 511      // n
  2 + [2] =   512 -> 1023     // nn
  3 + [3] =  1024 -> 2047     // nnn
  4 + [4] =  2048 -> 4095     // nnnn
  5 + [5] =  4096 -> 8191     // nnnnn
  6 + [6] =  8192 -> 16383    // nnnnnn
  7 + [7] = 16384 -> 32767    // nnnnnnn
  8 + [8] = 32768 -> 65535    // nnnnnnnn
  9 + [9] = 65536 -> 131071   // nnnnnnnnn <- provisional..

 alternative offset tANS symbols from 0 to 8 
  
  0 + [7]  =     1 -> 127     // nnnnnnn 
  1 + [8]  =   128 -> 255     // nnnnnnn+n
  2 + [9]  =   256 -> 511     // nnnnnnn+nn
  3 + [10] =   512 -> 1023    // nnnnnnn+nnn
  4 + [11] =  1024 -> 2047    // nnnnnnn+nnnn
  5 + [12] =  2048 -> 4095    // nnnnnnn+nnnnn
  6 + [13] =  4096 -> 8191    // nnnnnnn+nnnnnn
  7 + [14] =  8192 -> 16383   // nnnnnnn+nnnnnnn
  8 + [15] = 16384 -> 32767   // nnnnnnn+nnnnnnnn
  9 + [16] = 32768 -> 65535   // nnnnnnn+nnnnnnnnn



@endverbatim

*/

#define ZXPAC4C_INIT_PMR_OFFSET          5 
#define ZXPAC4C_MATCH_MIN                2
#define ZXPAC4C_MATCH_MAX                255
#define ZXPAC4C_MATCH_GOOD               63
#define ZXPAC4C_OFFSET_MATCH2_THRESHOLD  1024
#define ZXPAC4C_OFFSET_MATCH3_THRESHOLD  4096
#define ZXPAC4C_WINDOW_MAX               131072
#define ZXPAC4C_HEADER_SIZE              4

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
    int lz_encode(char* buf, int len, std::ofstream* ofs);

    const cost* lz_get_result(void) { return m_cost_array; }
    const cost* lz_cost_array_get(int len);
    void lz_cost_array_done(void);
    bool is_ascii(void) { return m_lz_config->is_ascii; }
    bool only_better(void) { return m_lz_config->only_better_matches; }
};


#endif  // _ZXPAC4C_H_INCLUDED
