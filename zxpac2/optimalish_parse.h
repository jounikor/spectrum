#ifndef _OPTIMALISH_PARSER_H_INCLUDED
#define _OPTIMALISH_PARSER_H_INCLUDED

#include <stding.h>
#include "zxpac_v2.h"

#define NULLHSH  -1
#define HSHSIZE 65536

typedef int32_t next_t;


int optimal_parser(uint8_t *src, int src_len, match_t *match,
    int match_len, int min_match, int good_match, int max_match, 
    int win_size, int depth, cost_function cost);






#endif

