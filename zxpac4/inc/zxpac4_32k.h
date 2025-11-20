/**
 * @file zxpac4_32k.h
 * @brief ZX Pac v4 with 32K window class definitions
 * @author Jouni 'Mr.SpivKorhonen
 * @version 0.3
 * @date fall 2024
 * @date sprint 2025
 * @copyright The Unlicense
 *
 *
 */


#ifndef _ZXPAC4A_32K_H_INCLUDED
#define _ZXPAC4A_32K_H_INCLUDED

#include <vector>
#include <cstdint>
#include "lz_base.h"
#include "lz_util.h"
#include "hash.h"
#include "cost4_32k.h"

/**
 The binary encoding of zxpac4_32k format:
@verbatim

 ASCII-only mode compression is possible if all charcters must be between 0 to 127.
 It will in most cases 1 bit per encoded literal or a match following a literal.
 Binary mode compression when all charaters are between 0 to 255.
 
  Ascii-only:
  0 + (LLLLLLLx), x = next tag bit.
  Binary:
  0 + (LLLLLLLL)
  
  PMR:
  1 + 1 + matchlen   -> PMR, where 0 < matchlen < 255
  
  Match:
  1 + 0 + (Xnnnnnnn) + reminder of offset (possible X bit) + matchlen
          Where matchlen + 1 is the final length

 matchlen:
  0               + [0] = 1                 // 0
  10              + [1] = 2 -> 3            // 1n0
  110             + [2] = 4 -> 7            // 1n1n0
  1110            + [3] = 8 -> 15           // 1n1n1n0
  11110           + [4] = 16 -> 31          // 1n1n1n1n0
  111110          + [5] = 32 -> 64          // 1n1n1n1n1n0
  1111110         + [6] = 64 -> 127         // 1n1n1n1n1n1n0
  1111111         + [7] = 128 -> 255        // 1n1n1n1n1n1n1n

 offset
  0       + [0]  = 1 -> 127                 // 0+nnnnnnn                    8
  100     + [0]  = 128 -> 255               // 1+nnnnnnn+00                 10
  101     + [1]  = 256 -> 511               // 1+nnnnnnn+01+n               11
  1100    + [2]  = 512 -> 1023              // 1+nnnnnnn+100+nn             13
  1101    + [3]  = 1024 -> 2047             // 1+nnnnnnn+101+nnn            14
  11100   + [4]  = 2048 -> 4095             // 1+nnnnnnn+1100+nnnn          16
  11101   + [5]  = 4096 -> 8191             // 1+nnnnnnn+1101+nnnnn         17
  11110   + [6]  = 8192 -> 16383            // 1+nnnnnnn+11100+nnnnnn       19
  11111   + [7]  = 16384 -> 32767           // 1+nnnnnnn+11101+nnnnnnn      20

  0+nnnnnnnn                        8
  1+nnnnnnnn + 0                    9
  1+nnnnnnnn + 1n0                  11
  1+nnnnnnnn + 1n1n0                13
  1+nnnnnnnn + 1n1n1n0              16
  1+nnnnnnnn + 1n1n1n1n0            17
  1+nnnnnnnn + 1n1n1n1n1n0          18
  1+nnnnnnnn + 1n1n1n1n1n1n0        21
  1+nnnnnnnn + 1n1n1n1n1n1n1n0      23
  1+nnnnnnnn + 1n1n1n1n1n1n1n1n0    25
  1+nnnnnnnn + 1n1n1n1n1n1n1n1n1n0  27

@endverbatim

*/

#define ZXPAC4_32K_INIT_PMR_OFFSET          5 
#define ZXPAC4_32K_MATCH_MIN                2
#define ZXPAC4_32K_MATCH_MAX                255
#define ZXPAC4_32K_MATCH_GOOD               63
#define ZXPAC4_32K_OFFSET_MATCH2_THRESHOLD  1024
#define ZXPAC4_32K_OFFSET_MATCH3_THRESHOLD  4096
#define ZXPAC4_32K_WINDOW_MAX               32768
#define ZXPAC4_32K_HEADER_SIZE              4

/**
 * @class matches utils.h
 * @brief A container class to hold all matches within the sliding window.
 * 
 *
 */

class zxpac4_32k : public lz_base {
    hash3 m_lz;
    cost* m_cost_array;
    match* m_match_array;
    int m_alloc_len;
    zxpac4_32k_cost m_cost;
private:
    int encode_history(const char* buf, char* out, int len, int pos);
public:
    zxpac4_32k(const lz_config* cfg, int ins=-1, int max=-1);
    ~zxpac4_32k();
    int lz_search_matches(char* buf, int len, int interval);
    int lz_parse(const char* buf, int len, int interval);
    int lz_encode(char* buf, int len, char* outb, std::ofstream* ofs);

    const cost* lz_get_result(void) { return m_cost_array; }
    const cost* lz_cost_array_get(int len);
    void lz_cost_array_done(void);
    bool is_ascii(void) { return m_lz_config->is_ascii; }
    bool only_better(void) { return m_lz_config->only_better_matches; }
};


#endif  // _ZXPAC4A_32K_H_INCLUDED
