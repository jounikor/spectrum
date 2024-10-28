/**
 * @file zxpac4b.h
 * @brief ZX Pac v4 class definitions
 * @author Jouni 'Mr.SpivKorhonen
 * @version 0.1
 * @date fall 2024
 * @copyright The Unlicense
 *
 *
 */


#ifndef _ZXPAC4B_H_INCLUDED
#define _ZXPAC4B_H_INCLUDED

#include <vector>
#include <cstdint>
#include "lz_base.h"
#include "lz_util.h"
#include "hash.h"
#include "cost4b.h"

/**
 The binary encoding of zxpac4 format:
@verbatim

 ASCII-only mode compression is possible if all charcters must be between 0 to 127.
 It will in most cases 1 bit per encoded literal or a match following a literal.
 Binary mode compression when all charaters are between 0 to 255.

 The PMR encoding uses the cool trick recently seen in Einar Saukas' ZX0 compressor.
 I am definitive I have seen it earlier used somewhere else (could be some of
 Charles Bloom's rants). Anyway, I have so far rejected the idea because the need
 of "run" of literals instead of a single literal at a time approach. It is easier
 to handle individual literals with statistical encoders.

 There's one limitation to the bit encoding below. A PMR after literal run cannot
 handle two or more PMRs in a row. This is a valid encoding with other zxpac4 
 encoders and happens time to time, specifically encoding two PMR literals after
 a normal encoded literal in ASCII mode. The encoding of two PMRs is cheaper than
 the encoding of a match of 2. The zxpac4b encoder takes this limitation into 
 account if the match cost calculator comes up with such sequence of PMRs and
 combines two PMRs into a one.


  Ascii-only:
  0 + (LLLLLLLx), x = next tag bit.
  Binary:
  0 + (LLLLLLLL)
  
  PMR (only applicable after a literal run):
  0 + matchlen   -> PMR, where 0 < matchlen < 255
  
  Match:
  1 + (Xnnnnnnn) + reminder of offset (possible X bit) + matchlen
          Where matchlen + 1 is the final length

 matchlen or literallen:
  0       + [0] = 1                     // 0
  10      + [1] = 2 -> 3                // 1n0
  110     + [2] = 4 -> 7                // 1n1n0
  1110    + [3] = 8 -> 15               // 1n1n1n0
  11110   + [4] = 16 -> 31              // 1n1n1n1n0
  111110  + [5] = 32 -> 64              // 1n1n1n1n1n0
  1111110 + [6] = 64 -> 127             // 1n1n1n1n1n1n0
  1111111 + [7] = 128 -> 255            // 1n1n1n1n1n1n1n
  (note: need to fix this max 255 literals..)

 offset
  0       + [0]  = 1 -> 127             // 0+nnnnnnn
  100     + [0]  = 128 -> 255           // 1+nnnnnnn+00
  101     + [1]  = 256 -> 511           // 1+nnnnnnn+01+n
  1100    + [2]  = 512 -> 1023          // 1+nnnnnnn+100+nn
  1101    + [3]  = 1024 -> 2047         // 1+nnnnnnn+101+nnn
  11100   + [4]  = 2048 -> 4095         // 1+nnnnnnn+1100+nnnn
  11101   + [5]  = 4096 -> 8191         // 1+nnnnnnn+1101+nnnnn
  111100  + [6]  = 8192 -> 16383        // 1+nnnnnnn+11100+nnnnnn
  111101  + [7]  = 16384 -> 32767       // 1+nnnnnnn+11101+nnnnnnn
  111110  + [8]  = 32768 -> 65535       // 1+nnnnnnn+11110+nnnnnnnn
  111111  + [9]  = 65536 -> 131071      // 1+nnnnnnn+11111+nnnnnnnnn
@endverbatim

*/

#define ZXPAC4B_OFFSET_MATCH2_THRESHOLD     1024
#define ZXPAC4B_OFFSET_MATCH3_THRESHOLD     4096
#define ZXPAC4B_WINDOW_MAX                  131072
#define ZXPAC4B_INIT_PMR_OFFSET             5

#define ZXPAC4B_MATCH_MIN                   2
#define ZXPAC4B_MATCH_MAX                   255
#define ZXPAC4B_MATCH_GOOD                  63
#define ZXPAC4B_HEADER_SIZE                 4

/**
 * @class matches utils.h
 * @brief A container class to hold all matches within the sliding window.
 * 
 *
 */

class zxpac4b : public lz_base {
    hash3 m_lz;
    cost* m_cost_array;
    int m_alloc_len;
    zxpac4b_cost m_cost;
private:
    int encode_history(const char* buf, char* out, int len, int pos);
public:
    zxpac4b(const lz_config* cfg, int ins=-1, int max=-1);
    ~zxpac4b();
    int lz_search_matches(char* buf, int len, int interval);
    int lz_parse(const char* buf, int len, int interval);
    int lz_encode(const char* buf, int len, std::ofstream& ofs);

    const cost* lz_get_result(void) { return m_cost_array; }
    const cost* lz_cost_array_get(int len);
    void lz_cost_array_done(void);
    bool is_ascii(void) { return m_lz_config->is_ascii; }
    bool only_better(void) { return m_lz_config->only_better_matches; }
};


#endif  // _ZXPAC4B_H_INCLUDED
