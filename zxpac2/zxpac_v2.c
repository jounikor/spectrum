///////////////////////////////////////////////////////////////////////////////
 //
// Scoopex ZX Spectrum Cruncher v0.2 - ZXPac
// (c) 2013-14 Jouni 'Mr.Spiv' Korhonen
// This code is for public domain (if you so insane to use it..), so don't
// complain about my no-so-good algorithm and messy one file implementation.
//
// Originally coded for the 'ScoopZX' intro. See the companion
// TAP creation tool. The TAP "spectrum exe-packer" tool is purposely separate
// as I wanted to reuse this code without hassle outside ZX Spectrum as well.
//
// The code implements one search algorithm that can be used for:
//  1) non-greedy parsing (variable distance)
//  2) lazy evaluation (distance == 2)
//  3) greedy parsing (distance = 1)
//
// The search 'engine' is made for all cases, therefore the searching speed
// is not that great (e.g. a lot of unnecessary searching is done as I naively
// do a match on every possible symbol on the file to be crunched).
// However, the search engine still implements a 'look into the future' hash
// table. The hash code is essentially the same I implemented in StoneCracker
// series of Amiga crunchers in early -91 or so ;) The octet aligned encoding
// also originates from my Amiga times somewhere in -90s and therefore is
// probably far from the state of the art.
//
///////////////////////////////////////////////////////////////////////////////
///
/// \file zxpac_v2.c
/// \brief A simple Z80 (ZX SPectrum) optimized compressor using good old 
///        LZSS algorithm.
///
/// \author Jouni Korhonen
/// \copyright (c) 2009-14 Jouni Korhonen
/// \version 2
///


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>

/// \struct hash
/// \brief This structure contains one hash cell, which is actually represented as
///        a linked list node. \c next is an index to the next linked list node and
///        \c last in an index to the last linked list node.
///
/// Note, the current design limits the sliding window to 64K
///

struct hash {
	uint16_t next;
	uint16_t last;
};

/// \struct match
/// \brief This structure contains \c off offset to the found match, \c len length
///        of the match and \c raw literal octet. The information is relative to 
///        the file position.
///

struct match {
	uint16_t off;
	short len;
	uint8_t raw;
    int8_t omidx;   // calculated om index from off & len
    //short idx;      // file index of this match structure
    int idx;
};

/// \struct om
/// \brief This structure holds the offset and matchlength information
///        used by the 'om index'.
///

struct om {
    int cnt;
    uint16_t tag;
    int8_t len;
    int8_t idx;
};



//
// The crunched file has the following layout, yet simple but allows
// 1) inplace decrunching and
// 2) short & fast decruncher code at least on Z80
//
// The crunched file is decrunched from end to beginning. Everything is
// octet aligned allowing fast decrunching with minimal bitshifting.
//
// File Layout:
//
//                76543210 76543210
// +------//-----+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
// |crunched data|   LO   |   HI   | om0/1  | om2/3  | om4/5  | om6/7  | om8/9  | om10/11|om12/13 |om14/15 |
// +------//-----+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
//
// Tags for the crunched data (read from end to beginning):
//
//  1-n Octets            76543210
// +---------//--------+ +--------+
// | '1' literal octet |+|0xxxxxxx| - tag 0
// +---------//--------+ +--------+
//
// Literals:
//  0 + [8]
//
// om indexes. These are actually a variation of Golomb codes.
//
//  100          om0
//  101          om1
//  1100         om2
//  1101         om3
//  11100        om4
//  11101        om5
//  111100       om6
//  111101       om7
//  1111100      om8
//  1111101      om9
//  11111100     om10
//  11111101     om11
//  111111100    om12
//  111111101    om13
//  111111110    om14
//  111111111    om15
//
// 'om index' is used for a lookup into a file specific table that
// contains offset + matchlen information.
//
// The 'om' index is a combination of two bit 'o' and two bit 'm' 
// information. Together these create a 'namespace' between 0 to 15.
// The looked up byte in the table indexed by the 'om' has 4 bits
// of input bit length for both offset and matchlen.
//
// The offset information tansforms to an offset as follows:
//
// oo = 0 -> [5]  offset 1 to 32		(i.e. 1<<0 + [5])
// oo = 1 -> [8]  offset 33 to 288		(i.e. 1<<0 + 1<<5 + [8])
// oo = 2 -> [11] offset 289 to 2336	(i.e. 1<<0 + 1<<5 + 1<<8 + [11])
// oo = 3 -> [14] offset 2337 to 18720	(i.e. 1<<0 + 1<<5 + 1<<8 + 1<<11 + [14])
//
// The matchlen information transforms to a length as follows:
//
//      m     n
// mm = 0 -> [1] matchlen 2 to 3		(i.e. 2 + [1])
// mm = 1 -> [3] matchlen 4 to 11		(i.e. 2 + 1<<1 + [3])
// mm = 2 -> [5] matchlen 12 to 43		(i.e. 2 + 1<<1 + 1<<3 + [5])
// mm = 3 -> [7] matchlen 44 to 171		(i.e. 2 + 1<<1 + 1<<5 + [7])
//

#define MM0 2
#define MM1 4
#define MM2 12
#define MM3 44
#define MM4 172
#define MM0B 1
#define MM1B 3
#define MM2B 5
#define MM3B 7

#define OO0 1
#define OO1 33
#define OO2 289
#define OO3 2337
#define OO4 18721
#define OO0B 5
#define OO1B 8
#define OO2B 11
#define OO3B 14

#define MINMTCH MM0
#define MAXMTCH (MM4-1)
//#define MAXMTCH (MM3-1)

// The compressor handles max 64K sized files..

#define BUFSIZE 2*65536
#define WINSIZE (OO4-1)


#define NULLHSH 0

// Other constants..

#define ALGO_GDY 0  /// <greedy parsing
#define ALGO_LZY 1  /// <lazy evaluation
#define ALGO_NON 2  /// <nongreedy parsing
#define NONMAX 32
#define NONDEF 8

//                       
static struct om omtable[16] = {0};
static uint8_t outom[8] = {0};

static struct hash head[65536];
static uint16_t next[WINSIZE+1];
static uint16_t ring;
static int greedy;
static int debug = 0;

//

static struct match matches[BUFSIZE];
static uint8_t buf[BUFSIZE];

static int encodeliteral(uint8_t lit, uint32_t *b ) {
    if (b) { *b = lit; }
    return 8;
}

static int getoo( int off ) {
    assert(off >= OO0 && off < OO4);
    if (off < OO1) {
        return 0;
    } else if (off < OO2) {
        return 1;
    } else if (off < OO3) {
        return 2;
    } else {
        return 3;
    }
}

static int encodeoff( int off, uint16_t *b ) {
    int oo = getoo( off );

    switch (oo) {
        case 0:
            if (b) { *b = (uint16_t)(off-OO0); }
            return OO0B;
        case 1:
            if (b) { *b = (uint16_t)(off-OO1); }
            return OO1B;
        case 2:
            if (b) { *b = (uint16_t)(off-OO2); }
            return OO2B;
        case 3:
            if (b) { *b = (uint16_t)(off-OO3); }
            return OO3B;
        default:
            assert(0);
    }
}

static int getmm( int len ) {
    assert(len >= MM0 && len < MM4);
    if (len < MM1) {
        return 0;
    } else if (len < MM2) {
        return 1;
    } else if (len < MM3) {
        return 2;
    } else {
        return 3;
    }
}

static int encodelen( int len, uint16_t *b ) {
    int mm = getmm( len );
    
    switch (mm) {
        case 0:
            if (b) { *b = (uint16_t)(len-MM0); }
            return MM0B;
        case 1:
            if (b) { *b = (uint16_t)(len-MM1); }
            return MM1B;
        case 2:
            if (b) { *b = (uint16_t)(len-MM2); }
            return MM2B;
        case 3:
            if (b) { *b = (uint16_t)(len-MM3); }
            return MM3B;
        default:
            assert(0);
    }
}

static void initom(void) {
    int o;

    for (o = 0; o < 16; o++) {
        omtable[o].cnt = 0;
        omtable[o].idx = o;
    }
}

static void initmatches() {
    int n;

    for (n = 0; n < BUFSIZE; n++) {
        matches[n].len = 0;
        //matches[n].idx = n;
    }
}

static void prepareom( struct match* m, struct om *omtab, int num ) {
    int n;

    for (n = 0; n < num; n++) {
        if (m[n].len > 0) {
            // 76543210
            // ----mmoo
            m[n].omidx = getmm(m[n].len) << 2 | getoo(m[n].off);
            ++omtab[m[n].omidx].cnt;
        }
    }
}

static int cmp_des( const void *a, const void *b ) {
    return ((struct om*)b)->cnt - ((struct om*)a)->cnt;
}
static int cmp_asc( const void *a, const void *b ) {
    return ((struct om*)a)->idx - ((struct om*)b)->idx;
}

static void calculateom( struct om tab[16], uint8_t out[8] ) {
    int n;
    uint16_t tag;
    int len;

    // sort by counts
    qsort(tab,16,sizeof(struct om),cmp_des);
   
    // sort by index within count pairs
    for (n = 0; n < 12; n += 2) {
        // somewhat lazy approach ;-)
        qsort(tab+n,2,sizeof(struct om),cmp_asc);
    }
    qsort(tab+n,4,sizeof(struct om),cmp_asc);

    // calculate tags and their lengths.

    tag = 0x04; //  00000100b
    len = 3;

    for (n = 0; n < 12; n += 2) {
        tab[n].tag = tag++;
        tab[n].len = len;
        tab[n+1].tag = tag++;
        tab[n+1].len = len++;
        tag <<= 1;
    }
    for (; n < 16; n++) {
        tab[n].tag = tag++;
        tab[n].len = len;
    }
    for (n = 0; n < 16; n += 2) {
        out[n >> 1] = tab[n].idx << 4 | tab[n+1].idx;
    }
    
    qsort(tab,16,sizeof(struct om),cmp_asc);
}

static uint32_t bb;
static int bc;
static uint8_t *bbyte;

static uint8_t *initputbits( uint8_t *b ) {
    bb = 0;
    bc = 8;
    bbyte = b;
    return b;
}

static uint8_t *putbyte( uint8_t o ) {
    *bbyte++ = o;
    return bbyte;
}


static uint8_t *putbits( uint16_t d, int l ) {
    assert(l > 0);
    if (l > 8) {
        int t = l-8;


        putbits(d & ((1 << t)-1) , t);    
        d >>= t;
        l = 8;
    }
    
    bb |= (d << 8);

    if (bc <= l) {
        bb >>= bc;
        *bbyte++ = bb;
        l -= bc;
        bc = 8;
    }
    bc -= l;
    bb >>= l;
    return bbyte;
}

static uint8_t *flushputbits( void ) {
    if (bc == 8) {
        bb = 0x80;
    } else {
        bb &= (0xff << bc);
        bb |= (0x80 >> 8-bc);
    }
    *bbyte++ = bb;
    return bbyte;
}


//
//
//
//

static void initHash( void ) {
	memset(head,0,sizeof(head));
	memset(next,0,sizeof(next));
	ring = 1;
};

//
// Direct 16bits hash "function"

static uint16_t calcHash( int n ) {
	return buf[n] << 8 | buf[n+1];
}

//
// Delete the first hash cell in the linked list of hash cells pointed
// by our hash value. Return the index to the deleted hash cell.

static int deleteHash( uint16_t hash ) {
	// first delete an overwritte hash slot
	uint16_t nxt = head[hash].next;

	if ((head[hash].next = next[nxt]) == NULLHSH) {
		head[hash].last = NULLHSH;
	}
	return head[hash].next;
}


static void insertHash( uint16_t hash ) {
	// insert a new slot
	next[ring] = NULLHSH;
	
	// update hash head table
	next[head[hash].last] = ring;
	head[hash].last = ring;

	if (head[hash].next == NULLHSH) {
		head[hash].next = ring;
	}
	if (++ring > WINSIZE) {
		ring = 1;
	}
}

static void insertHashDummy( void ) {
	if (++ring > WINSIZE) {
		ring = 1;
	}
}



static void getMatch( int n, uint16_t nxt, struct match *mch, int end ) {
	int len, off, fut, i;
    float bbp = 32.0;

	uint8_t *sp, *cp, *hp, *me, *bs;
	len = MINMTCH-1;

	// current search position.
	sp = buf + n;
	// Calculate search area end..
	me = buf + end;

	while (nxt) {
        if (ring > nxt) {
            fut = WINSIZE - ring + nxt;
        } else {
            fut = nxt - ring; 
		}
        //if ((fut = nxt - ring) < 0) {
		//	fut += WINSIZE;
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

		i = n+fut+MAXMTCH < end ? MAXMTCH : end-n-fut;

		while (i-- > 0 && *cp == *hp) {
			++cp; ++hp;
		}

		if ((hp - bs) > len) {
            len = hp - bs;
            off = fut;
        }
		if (len >= MAXMTCH) {
			break;
		}
		nxt = next[nxt];
	}

    // discard pathetic matches..
    if ((len == MINMTCH && off >= OO3) || len < MINMTCH) {
        len = 0;
    }

	mch->raw = buf[n];
	mch->len = len;
	mch->off = off;
}

//
// Parameters:
//  n [in]   - Current file position
//  end [in] - End of file
//
//  Note that 'end' must be calculated so that a lookup into the
//  tables does not overflow!
//
// Returns:
//  Next position in the file.
//

static int nonGreedy( int n, int nonmax, struct match *outm  ) {
	int dlt, max ,j, r;

	// The theory behind the non-greedy parsing has been
	// explained nicely in Nigel Horspool's old paper 
	// "The effect of non-greedy pasing in Ziv-Lempel
	//  compression methods". Read that :)
		
	if (matches[n].len < MINMTCH) {
		dlt = 1;
    } else {
		dlt = 0;
        //max = matches[n].len-1 < nonmax ? matches[n].len-1 : nonmax;
        max = matches[n].len < nonmax ? matches[n].len : nonmax;
        //max = matches[n].len-1 < NONMAX ? matches[n].len-1 : NONMAX;
        //if (n+max >= end) {
		//	max = end-n-1;
		//}
		// for (j = 1; j < max; j++) {
		for (j = 1; j < max; j++) {
            if (matches[n+j].len > (matches[n+dlt].len+j)) {
                dlt = j;
                if (debug) {
                    printf("** n: %d max: %d, dlt: %d, nonmax: %d\n",
                        n, max,dlt,nonmax);
                }
			}
		}
	}
    
    *outm = matches[n];
	
    if (dlt == 0) {
        // no changes..
		r = matches[n].len;
	} else {
        if (dlt == 1) {
		    r = 1;	
            outm->len = 0;
	    } else {
            outm->len = dlt;
		    r = dlt;
	    }
	}
	return r;
}

//
//
// Parameters:
//  sta [in] - Current file position
//  end [in] - End of file
//  b [out]  - Buffer for encoded (crunched) data
//
// Returns:
//  Length of encoded data.
//

int encode( uint8_t *b, int s, struct match *m, int n,
            struct om *ot, uint8_t *ob, int orig ) {
	
    int i, j, k, omidx;
    uint16_t tmp;
    uint8_t *sta = b;
    uint32_t realpos = 0;

    initputbits(b);

    if (debug) {
        puts("**Endocing starts**");
    }

    for (i = 0; i < n; i++) {
        if (m[i].len == 0) {
            putbyte(m[i].raw);  // [8]
            putbits(0,1);       // tag 0
            if (debug) {
                printf("%04X (%5d) RAW %02X ('%c')\n",realpos,m[i].idx,m[i].raw,
                    isprint(m[i].raw) ? m[i].raw : '.');
            }
            ++realpos;
        } else {
            omidx = m[i].omidx;
            // output length..
            j = encodelen(m[i].len,&tmp);
            putbits(tmp,j);
          
            if (debug) {
                printf("%04X (%5d) MTC %d,%d OM %02X -> %04x (%d)\n",realpos, m[i].idx,
                    m[i].off,m[i].len,
                    m[i].omidx, ot[omidx].tag,ot[omidx].len);
            }
            realpos += m[i].len;

            // output offset..
            j = encodeoff(m[i].off,&tmp);
            k = j >= 8 ? j - 8 : j;
            
            if (k > 0) {
                putbits(tmp & ((1 << k)-1),k);
            } 
            if (j >= 8) {
                putbyte(tmp>>k);
            }
            
            // output tag.. between 3 to 9 bits{
            putbits(ot[omidx].tag,ot[omidx].len);
        }
    }
    b = flushputbits();
    // original length
    *b++ = orig;
    *b++ = orig >> 8;
    // om information
    for (i = 0; i < 8; i++) {
        *b++ = ob[i];
    }
    
    return (int)(b-sta);
}

//
//

static struct option longopts[] = {
	{"gry", no_argument, NULL, 'g'},
	{"lzy", no_argument, NULL, 'l'},
	{"non", no_argument, NULL, 'n'},
	{"debug", no_argument, NULL, 'D'},
	{"len", required_argument, NULL, 'N'}
};

//

void usage( char *prg ) {
	fprintf(stderr,"Usage: %s [options] infile outfile\n",prg);
	fprintf(stderr," Options:\n"
				   "  --gry     Selects greedy matching\n"
				   "  --lzy     Selects lazy evaluation matching\n"
				   "  --non     Selects non-greedy matching (defaults to %d)\n"
				   "  --debug   Print debug output (will be plenty)\n"
                   "  --len num Selects non-greedy matching with num distance\n", NONDEF);
}
 
//
//

int main( int argc, char **argv ) {
	FILE *inf, *outf;
	int algo;
	int flen;
	int n, p, o;
	uint16_t hsh;
	uint16_t nxt;

	fprintf(stderr, "ZXPac v0.2 - (c) 2009-13 Mr.Spiv of Scoopex\n");

	// set up stuff
	inf = NULL;
	outf = NULL;
	algo = ALGO_NON;

	//
	//

	while ((n = getopt_long(argc, argv, "DlN:gn", longopts, NULL)) != -1) {
		switch (n) {
			case 'l':
				algo = ALGO_LZY;
                greedy = 2;
				break;
			case 'N':
				algo = ALGO_NON;
                greedy = atoi(optarg);
				break;
			case 'g':
				algo = ALGO_GDY;
                greedy = 1;
				break;
			case 'n':
				algo = ALGO_NON;
                greedy = NONDEF;
				break;
            case 'D':
                debug = 1;
                break;
			case '?':
			case ':':
				usage(argv[0]);
				exit(EXIT_FAILURE);
				break;
			default:
				break;
		}
	}

	if (argc - optind < 2) {
	//if (argc - optind < 1) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if ((inf = fopen(argv[optind], "r+b")) == NULL) {
		fprintf(stderr, "**Error: fopen(%s) failed: %s\n",argv[optind],
				strerror(errno));
		exit(EXIT_FAILURE);
	}
	if ((outf = fopen(argv[optind+1], "wb")) == NULL) {
		fclose(inf);
        fprintf(stderr, "**Error: fopen(%s) failed: %s\n",argv[optind+1],
				strerror(errno));
		exit(EXIT_FAILURE);
	}
    if (greedy < 1) {
        fprintf(stderr,"**Warning: minimum non-greedy distance 1 set\n");
        greedy = 1;
    }
    if (greedy > NONMAX) {
        fprintf(stderr,"**Warning: maximum non-greedy distance %d set\n",NONMAX);
        greedy = NONMAX;
    }

	//
	//

    if (debug) {
        printf("nonGreedy queue length = %d\n",greedy);
    }

	flen = fread(buf,1,BUFSIZE,inf);

	if (flen < 0) {
		fprintf(stderr,"** Error: fread() failed: %s\n", strerror(errno));
		fclose(inf);
		fclose(outf);
		exit(EXIT_FAILURE);
	}
	if (flen < 2) {
		fprintf(stderr, "**Error: file too short.\n");
		fclose(inf);
		fclose(outf);
		exit(EXIT_FAILURE);
	}

	// Start compression..
	//
	
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

    // calculate om..

    initom();
    prepareom(matches,omtable,matchHead);
    calculateom(omtable,outom);

    //

    if (debug) {
        printf("\n**** OM bit counts before softing *****\n\n");
        for (o = 0; o < 16; o++) {
            printf("[%d]: %d -> %d \n",o,omtable[o].cnt,omtable[o].idx);
        }

        printf("\n**** OM bit counts after softing *****\n\n");
        
        for (o = 0; o < 16; o++) {
            printf("[%2d]: %2d -> %2d tag: %04x (%d) \n",o,omtable[o].cnt,omtable[o].idx,
                omtable[o].tag,omtable[o].len);
        }
        printf("\n**** Output OM *****\n\n");
        for (n = 0; n < 8; n++) {
            printf("Octet %d -> %02x\n",n,outom[n]);
        }
    }

	// Encode literals + matches
	n = encode(buf,65536,matches,matchHead,omtable,outom,flen);

	// encode original length

	fprintf(stderr,"Crunched length: %d (%2.2f%%)\n",n,(float)(100.0 * (flen-n) / flen));
#if 1
	flen = fwrite(buf,1,n,outf);

	if (flen != n) {
		fprintf(stderr,"** Error: fwrite() failed: %s\n", strerror(errno));
	}
#endif
	fclose(inf);
	fclose(outf);
	exit(EXIT_SUCCESS);
}

/* vim: cindent */
