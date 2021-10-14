/*
 *
 *
 *
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "optimalish_parse.h"






/** \struct hash
 *  \brief This structure contains one hash cell, which is actually represented as
 *        a linked list node. \c next is an index to the next linked list node and
 *  \c last in an index to the last linked list node.
 *
 */

typedef struct hash {
	next_t next;
	next_t last;
} hash_t;

static hash_t *p_head = NULL;   // [65536];
static int32_t *p_next = NULL; //[WINSIZE+1];
static next_t s_ring;
static next_t s_winsize = 0;
static int s_minmatch = 0;
static uint8_t *p_buf = NULL;


//
//
//
//

static void done_hash(void)
{
    if (p_head) {
        free(p_head);
        p_head = NULL;
    }
    if (p_next) {
        free(p_next);
        p_next = NULL;
    }
    s_winsize = 0;
}


static bool init_hash(next_t winsize)
{
    if (s_winsize != winsize) {
        if (s_winsize != 0) {
            done_hash();
        }
    
        p_head = malloc(HSHSIZE * sizeof(hash_t));
        p_next = malloc((winsize) * sizeof(int32_t)); 
    
        if (p_head == NULL || p_next == NULL) {
            done_hash();
            return false;
        }

        s_winsize = winsize;
        s_ring = 0;
    }

    for (int n = 0; n < HSHSIZE; n++) {
        p_head[n].next = NULLHSH; 
    }
    for (int n = 0; n < winsize; n++) {
        p_next[n] = NULLHSH; 
    }
    
    s_ring = 0;
    return true;
};


//
// Direct 16bits hash "function"

static uint32_t calc_hash(uint8_t *p_buf,  int n ) {
	return (uint32_t)(p_buf[n] << 8 | p_buf[n+1]);
}

//
// Delete the first hash cell in the linked list of hash cells pointed
// by our hash value. Return the index to the deleted hash cell.

static next_t delete_hash( next_t hsh ) {
	// first delete an overwritte hash slot
	int32_t nxt = p_head[hsh].next;

	if ((p_head[hsh].next = p_next[nxt]) == NULLHSH) {
		p_head[hsh].last = NULLHSH;
	}
	return p_head[hsh].next;
}


// here.. FIX

static void insert_hash( next_t hsh ) {
	// insert a new slot
	p_next[s_ring] = NULLHSH;
	
	// update hash head table
	p_next[p_head[hsh].last] = s_ring;
	p_head[hsh].last = s_ring;

	if (p_head[hsh].next == NULLHSH) {
		p_head[hsh].next = s_ring;
	}

    // If winsize is power of 2 then a simple mask would work..
	if (++s_ring >= s_winsize) {
		s_ring = 0;
	}
}

static void insert_hash_dummy( void ) {
	if (++s_ring >= s_winsize) {
		s_ring = 0;
	}
}



static void get_match( next_t n, next_t nxt, match_t *mch, int end, int MAXMATCH ) {
	int len, off, fut, i;
    float bbp = 32.0;

	uint8_t *sp, *cp, *hp, *me, *bs;
	len = s_minmatch-1;

	// current search position.
	sp = p_buf + n;
	// Calculate search area end..
	me = p_buf + end;

	while (nxt != NULLHSH) {
        if (s_ring > nxt) {
            fut = s_winsize - s_ring + nxt;
        } else {
            fut = nxt - s_ring; 
		}
        //if ((fut = nxt - s_ring) < 0) {
		//	fut += s_winsize;
		//}
		if (n+fut >= end) {
			break;
			assert(n+fut < end);
		}

		// sp - current search position
		// cp - copy of sp during string matching
		// hp - ahead search position
		// me - search area end

		hp = sp + fut;
		bs = hp;
		cp = sp;

		i = n+fut < end ? MAXMATCH : end-n-fut;

		while (i-- > 0 && *cp == *hp) {
			++cp; ++hp;
		}

		if ((hp - bs) > len) {
            len = hp - bs;
            off = fut;
        }
		if (len >= MAXMATCH) {
			break;
		}
		nxt = p_next[nxt];
	}

    // discard pathetic matches..
    if (len < s_minmatch) {
        len = 0;
    }

	//mch->raw = buf[n];
	mch->len = len;
	mch->off = off;
}




#if 0	
	initHash();

	// fill in the hash table for the amount of sliding window.
	// note that if the actual input file is less that the
	// window size, we need to fill the rest of the hash table
	// with "dummy inserts".

	for (p = n = 0; n < WINSIZE; n++) {
		if (n < flen) {
			insertHash(calcHash(p++));
		} else {
			// updates the hash table and sliding window internals..
			insertHashDummy();
		}
	}

    // 'p' must be preserved.. since the search routine uses it.

    initmatches();

    int matchQueue = 0;
    int matchesToHash = 0;
    int matchHead = 0;
    int matchNext = 0;
    int len;
    int realPos = 0;

    n = 0;

    while (realPos < flen) {
        while (matchQueue < greedy && n < flen) {
            hsh = calcHash(n);
            nxt = deleteHash(hsh);
            getMatch(n, nxt, &matches[matchNext], flen);
            matches[matchNext].idx = n; // debug
            matchQueue++;
            matchNext++;
            n++;

            if (p < flen) {
                insertHash(calcHash(p++));
            } else {
                insertHashDummy();
            }
        }

        assert(matchQueue == matchNext-matchHead);
        assert(realPos == matches[matchHead].idx);
        len = nonGreedy(matchHead,matchQueue,&matches[matchHead]);
        
        if (len > 1 && memcmp(&buf[realPos],&buf[matches[matchHead].off + realPos],len)) {
            printf("mismatch at %04X -> %04X, off %d, len %d, mh %d, mq %d\n",
                realPos, matches[matchHead].off + realPos, matches[matchHead].off, len,
                matchHead,matchQueue);
        }
        
        matchHead++;
        realPos += len;
        
        if (len == 1) {
            matchQueue--;
        } else if (len < matchQueue) {
            int i, j;

            for (j = matchHead, i = matchHead+len-1; i < matchNext; i++) {
                matches[j++] =matches[i];
            }

            matchNext = j;
            matchQueue -= len;
        } else /*(len >= matchQueue)*/ {
            for (o = 0; o < len - matchQueue; o++) {
                hsh = calcHash(n++);
                nxt = deleteHash(hsh);
                if (p < flen) {
                    insertHash(calcHash(p++));
                } else {
                    insertHashDummy();
                }
            }
            matchNext = matchHead;
            matchQueue = 0;
        }
    }

#endif





/**
 *
 *
 *
 *
 */
int optimal_parser(uint8_t *p_buf, int buf_len, match_t *p_mtch, int match_len,
    int min_match, int mid_match, int max_match, int win_size,
    int depth, cost_function cost)
{

    printf("Entered optimal parser..\n");
    printf("src = %p\n",p_buf);
    printf("src_len = %d\n",buf_len);
    printf("match = %p\n",p_mtch);
    printf("match_len = %d\n",match_len);
    printf("min_match = %d\n",min_match);
    printf("mid_match = %d\n",mid_match);
    printf("max_match = %d\n",max_match);
    printf("win_size = %d\n",win_size);
    printf("depth = %d\n",depth);
    printf("cost = %p\n",cost);


    return 0;
}
