///////////////////////////////////////////////////////////////////////////////
//
// Scoopex ZX Spectrum Cruncher v0.22 - ZXPac
// (c) 2013-14/21 Jouni 'Mr.Spiv' Korhonen
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
/// \copyright (c) 2009-14/21 Jouni Korhonen
/// \version 0.22
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
#include <stdbool.h>
#include <ctype.h>

#include "z80dec.h"
#include "zxpac_v2.h"
#include "optimalish_parse.h"

#define VERSION "0.23"

/// \struct match
/// \brief This structure contains \c off offset to the found match, \c len length
///        of the match and \c raw literal octet. The information is relative to 
///        the file position.
///


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
// +------//-----+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
// |crunched data|op LO-IP|op HI-IP|   LO   |   HI   | om0/1  | om2/3  | om4/5  | om6/7  | om8/9  | om10/11|om12/13 |om14/15 |
// +------//-----+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
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
//  1+00          om0
//  1+01          om1
//  1+100         om2
//  1+101         om3
//  1+1100        om4
//  1+1101        om5
//  1+11100       om6
//  1+11101       om7
//  1+111100      om8
//  1+111101      om9
//  1+1111100     om10
//  1+1111101     om11
//  1+11111100    om12
//  1+11111101    om13
//  1+11111110    om14
//  1+11111111    om15
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



// Other constants..

#define NONMAX 32
#define NONDEF 0

static int om_cost(uint8_t *buf, int pos, int len)
{

    return 0;
}




static struct compressor_setting {
    lz_parser parser;
    cost_function cost;
    int min_match;
    int mid_match;
    int max_match;
    int win_size;
    int depth;
} settings[] = {
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 0
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 1
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 2
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 3
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 4
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 5
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 6
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 7
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 8
    {optimal_parser,om_cost,MM0,MM2,MM4,OO4,MM4},   // 9
    {NULL,NULL,0,0,0,0,0}
};






//                       
static struct om omtable[16] = {0};
static uint8_t outom[8] = {0};

static int greedy;
static bool debug = false;
static bool inplace = false;
static bool exe = false;

static uint32_t load_addr = 0x8000;
static uint32_t jump_addr = 0x8000;
static uint32_t page      = 0x7f;

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
    return -1;
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
    return -1;
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
            //m[n].omidx = getmm(m[n].len) << 2 | getoo(m[n].off);
            //++omtab[m[n].omidx].cnt;
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
        bb |= (0x80 >> (8-bc));
    }
    *bbyte++ = bb;
    return bbyte;
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

#if 0

int encode( uint8_t *b, int s, struct match *m, int n,
            struct om *ot, uint8_t *ob, int orig,
            uint32_t *inplacebytes) {
	
    int i, j, k, omidx;
    uint16_t tmp;
    uint8_t *sta = b;
    uint32_t realpos = 0;
    uint32_t ipbytes = 0;
    bool iptest = inplace;

    initputbits(b);

    if (debug) {
        puts("**Endocing starts**");
    }

    for (i = 0; i < n; i++) {
        if (m[i].len == 0) {
            putbyte(m[i].raw);  // [8]

            if (!iptest) {
                putbits(0,1);       // tag 0
            } else {
                ++ipbytes;
            }

            if (debug) {
                printf("%04X (%5d) RAW %02X ('%c')\n",realpos,m[i].idx,m[i].raw,
                    isprint(m[i].raw) ? m[i].raw : '.');
            }
            ++realpos;
        } else {
            iptest = false;
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
    // original length minus inplace bytes
    if (inplace) {
        *b++ = (orig - ipbytes);
        *b++ = (orig - ipbytes) >> 8;
    }
    // original length
    *b++ = orig;
    *b++ = orig >> 8;
    // om information
    for (i = 0; i < 8; i++) {
        *b++ = ob[i];
    }
    
    *inplacebytes = ipbytes;
    return (int)(b-sta);
}

#endif

//
//

static struct option longopts[] = {
	{"debug", no_argument, NULL, 'D'},
	{"sec", required_argument, NULL, 's'},
	{"lev", required_argument, NULL, 'L'},
    {"inplace", no_argument, NULL, 'i'},
    {"exe",required_argument, NULL, 'e'},
    {0,0,0,0}
};

//

void usage( char *prg ) {
	fprintf(stderr,"Usage: %s [options] infile outfile\n",prg);
	fprintf(stderr," Options:\n"
				   "  --lev   n   Compression level, Where n is a number between\n"
                   "              0 to 9 and 0 being fastest and 9 best compression\n"
                   "              Default value is %d\n"
                   "  --debug     Print debug output (will be plenty)\n"
                   "  --sec num   Security distance of num bytes for inplace decompression\n"
                   "  --exe load,jump,page Self-extracting decruncher parameters\n"
                   "  --inplace   Ensure inplace decrunching\n",NONDEF);
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
    int eff = NONDEF;
    int sec = 0;

	fprintf(stderr, "ZXPac v%s - (c) 2009-13/21 Mr.Spiv of Scoopex\n",VERSION);

	// set up stuff
	inf = NULL;
	outf = NULL;

	//
	//

	while ((n = getopt_long(argc, argv, "DlN:gnie:", longopts, NULL)) != -1) {
		switch (n) {
        case 'L':
            eff = atoi(optarg);
            
            if (eff < 0 || eff > 9) {
                fprintf(stderr,"--level parameters out of range.\n");
                exit(EXIT_FAILURE);;
            }
            break;
        case 's':
            sec = atoi(optarg);
            
            if (sec < 0) {
                fprintf(stderr,"--sec parameters out of range.\n");
                exit(EXIT_FAILURE);;
            }
            break;
        case 'D':
            debug = true;
            break;
        case 'e':
            n = sscanf(optarg,"%x,%x,%x",&load_addr,&jump_addr,&page);

            if (n != 3) {
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            if (load_addr > 0xffff ||
                jump_addr > 0xffff ||
                page > 0xff) {
                fprintf(stderr,"--exe parameters out of range.\n");
                exit(EXIT_FAILURE);;
            }

            //printf("%04x, %04x, %02x\n",load_addr,jump_addr,page);
            exe = true;
        case 'i':
            inplace = true;
            break;
        case '?':
        case ':':
            usage(argv[0]);
            exit(EXIT_FAILURE);
        default:
            break;
		}
	}

	if (argc - optind < 2) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
   
    // check for parsers..

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
    
	//
	//

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

    //

    lz_parser parser = settings[eff].parser;
    int idx = parser(buf,1,NULL,2,
                settings[eff].min_match,
                settings[eff].mid_match,
                settings[eff].max_match,
                settings[eff].win_size,
                settings[eff].depth,
                settings[eff].cost);


    // calculate om..

    if (debug) {
        printf("\n**** OM bit counts before sorting *****\n\n");
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

    uint32_t ipbytes = 0;

	//n = encode(buf,65536,matches,matchHead,omtable,outom,flen,&ipbytes);

	// encode original length

	fprintf(stderr,"Crunched %s file length: %d (%2.2f%%) (%d stored%s)\n",
        exe ? "self-extracting" : "data",
        n,(float)(100.0 * (flen-n) / flen),
        ipbytes, ipbytes > 0 ? " - use inplace decrunching" : "");

    flen = 0;

    if (exe) {
        set_load_addr(load_addr);
        set_jump_addr(jump_addr);
        set_work_addr((page << 8) | WORKSIZE);
        set_code_addr(load_addr+DECOMPRESS);
        set_data_size(n);
    
        flen = fwrite(z80exedec,1,sizeof(z80exedec),outf);
    
        if (flen != sizeof(z80exedec)) {
            goto file_error;
        }

        fprintf(stderr,"\tFinal length %d\n",n+flen);
    }

	flen = fwrite(buf,1,n,outf);

	if (flen != n) {
file_error:
		fprintf(stderr,"** Error: fwrite() failed: %s\n", strerror(errno));
	}

	fclose(inf);
	fclose(outf);
	exit(EXIT_SUCCESS);
}

/* vim: cindent */
