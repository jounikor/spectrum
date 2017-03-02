///////////////////////////////////////////////////////////////////////////////
//
// Scoopex ZX Spectrum Cruncher v0.1 - ZXPac
// (c) 2009 Jouni 'Mr.Spiv' Korhonen
// This code is for public domain (if you so insane to use it..), so don't
// complain about my no-so-good algorithm and messy one file implementation.
//
// Originally coded for the 'ScoopZX' intro. See the companion
// TAP creation tool. The TAP "spectrum exe-packer" tool is purposely separate
// as I wanted to reuse this code without hassle outside ZX Spectrum as well.
//
// The code implements two search algorithms:
//  1) non-greezy matching
//  2) optimal parsing
//
// The search 'engine' is made for all cases, therefore the searching speed
// is not that great (e.g. a lot of unnecessary searching is done as I naively
// do a match on every possible symbol on the file to be crunched).
// However, the search engine still implements a 'look into the future' hash
// table. The hash code is essentially the same I implemented in StoneCracker
// series of Amiga crunchers in early -90s ;) The octet aligned encoding
// also originates from my Amiga times somewhere in -90s and therefore is
// probably far from the state of the art.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>

//
//
//

struct hash {
	int16_t next;
	int16_t last;
};

struct match {
	u_int16_t off;
	int16_t len;
	u_int16_t raw;
};

// The compressor handles max 64K sized files..
// The sliding window is only 2K

#define BUFSIZE 65536
#define WINSIZE 2048		//1024
#define MINMTCH 2
#define MAXMTCH 255+15+2	//255+31+2

#define NULLHSH 0
#define MAXRAWS 63
#define SHRTOFF 32
#define LOWMTCH 3
#define MIDMTCH 17			//33

// Other constants..

#define ALGO_OPT 0
#define ALGO_NON 1

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
//                                        76543210 76543210
// +--------------//---------------------+--------+--------+
// | crunched data - (PHI<<8)+PLO octets |   PLO  |   PHI  |
// +--------------//---------------------+--------+--------+
//
// Tags for the crunched data (read from end to beginning):
//
//  76543210
// +--------+
// |00000000| - End of crunched data mark
// +--------+
//
//
//  1-n Octets                      76543210
// +--------------//---------------+--------+
// | 'n' uncrunched literals bytes |00nnnnnn| - 1 <= nnnnnn <= 63
// +--------------//------------------------+
//
//
//  76543210
// +--------+
// |01nxxxxx| - Match length = 2+n
// +--------+   Match offset = xxxxx+1
//
//
// When match length >= 2 but < 17 i.e. nnnnn < 0x0f
//
//  76543210 76543210
// +--------+--------+
// |zzzzzzzz|1nnnnxxx| - Match length = nnnn+2
// +--------+--------+   Match offset = ((x<<8)|z)+2
//
//
// When match length >= 17 but <= 272
//
//  76543210 76543210 76543210
// +--------+--------+--------+
// |zzzzzzzz|nnnnnnnn|11111xxx| - Match length = 15+nnnnnnnn+2
// +--------+--------+--------+   Match offset = ((x<<8)|z)+2
//
//
//

static struct hash head[65536];
static int16_t next[WINSIZE+1];
static struct match matches[BUFSIZE];
static u_int8_t buf[BUFSIZE];
static u_int16_t rawCnt[MAXRAWS];
static float output[MAXMTCH+1];
static float out[BUFSIZE+1];

static u_int16_t ring;

//
//
//
//

void initHash( void ) {
	memset(head,0,65536*sizeof(struct hash));
	memset(matches,0,BUFSIZE*sizeof(struct match));
	memset(next,0,WINSIZE*sizeof(int16_t));
	memset(rawCnt,0,sizeof(rawCnt));
	out[BUFSIZE] = 0;
	ring = 1;
};

//
// Direct 16bits hash "function"

u_int16_t calcHash( int n ) {
	return buf[n] << 8 | buf[n+1];
}

//
// Delete the first hash cell in the linked list of hash cells pointed
// by our hash value. Return the index to the deleted hash cell.

int16_t deleteHash( u_int16_t hash ) {
	// first delete an overwritte hash slot
	int16_t nxt = head[hash].next;

	if ((head[hash].next = next[nxt]) == NULLHSH) {
		head[hash].last = NULLHSH;
	}
	return head[hash].next;
}


void insertHash( int n, u_int16_t hash ) {
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

void insertHashDummy( void ) {
	if (++ring > WINSIZE) {
		ring = 1;
	}
}



void getMatch( int n, int16_t nxt, struct match *mch, int end ) {
	int len, off, fut, i;

	u_int8_t *sp, *cp, *hp, *me, *bs;
	len = 0;

	// current search position.
	sp = buf + n;
	// Calculate search area end..
	me = buf + end;

	while (nxt) {
		if ((fut = nxt - ring) < 0) {
			fut += WINSIZE;
		}
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
		if (len == MAXMTCH) {
			break;
		}
		nxt = next[nxt];
	}

	mch->raw = buf[n];
	mch->len = len >= 2 ? len : 0;
	mch->off = off;
}

//
//
// Parameters:
//  n [in]   - Current file position
//  end [in] - End of file
//
// Returns:
//  Next position in the file.
//

int optimalParser( int n, int end ) {

	int r, l, i, o;
	
	// The idea of Optimal Parsing is from Storer and Szymanski. I read
	// about it from Charles Bloom's web site..
	
	l = matches[n].len;
	o = matches[n].off;
	
	// Find out which match combination procudes the shortest
	// 'bits per byte'. Note that this minimizing process bascially
	// leaves the codes for cases 'offset > SHTROFF && match <=
	// LOWMTCH unused. Those could be reused for some other purpose..  

	for (i = MINMTCH; i <= l; i++) {
		if (i <= LOWMTCH && o <= SHRTOFF) {
			output[i] = (1.0 / i) + out[n+i];
		} else {
			if (i <= MIDMTCH) {
				output[i] = (2.0 / i) + out[n+i];
			} else {
				output[i] = (3.0 / i) + out[n+i];
			}
		}
	}
	
	// code length for literals.. this is slightly incorrect as
	// our literal encoding achieves in the worst case 2 bits for
	// each encoded bit and in the best case 1.02 bits for each
	// encoded bit.. minimum encoding is always 2 bytes though.
	// Who cares..
	
	output[1] = 1.00 + out[n+1];

	// Find an optimal coding.. for output[]
	
	for (r = 1, i = 2; i <= l; i++) {
		if (output[i] <= output[r]) {
			r = i;
		}
	}
	
	// store the optimal match..
	
	matches[n].len = r;
	out[n] = output[r];
	return n-1;
}


//
//
// Parameters:
//  n [in]   - Current file position
//  end [in] - End of file
//
// Returns:
//  Next position in the file.
//

int nonGreedy( int n, int end  ) {
	int dlt, max ,j, r;

	// The theory behind the non-greedy parsing has been
	// explained nicely in Nigel Horspool's old paper 
	// "The effect of non-greedy pasing in Ziv-Lempel
	//  compression methods". Read that :)
		
	if (matches[n].len < MINMTCH) {
		dlt = 1;
	} else {
		dlt = 0;
		max = matches[n].len-1;
		if (n+max >= end) {
			max = end-n-1;
		}
		for (j = 1; j < max; j++) {
			if (matches[n+j].len > (matches[n+dlt].len+j+1)) {
				dlt = j;
			}
		}
	}
	if (dlt == 0) {
		dlt = matches[n].len;
		r = dlt;
	} else if (dlt == 1) {
		dlt = 0;
		r = 1;	
	} else {
		r = dlt;
	}
	matches[n].len = dlt;
	return n+r;
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

int encode( int sta, int end, u_int8_t *b ) {
	int p, i, c, m;
	
	c = 0;
	p = 0;
	
	// End mark..
	b[c++] = 0x00;
	
	// Encode matches and literals..
	for (i = sta; i < end;) {
		if (matches[i].len < MINMTCH) {
			rawCnt[p++] = matches[i].raw;
			
			if (p == MAXRAWS) {
				// Tag: 00nnnnnn
				for (m = 0; m < p; m++) {
					b[c++] = rawCnt[m];
				}
				b[c++] = p;
				p = 0;
			}
			++i;
			continue;
		}
		
		if (p > 0) {
			// Tag: 00nnnnnn
			for (m = 0; m < p; m++) {
				b[c++] = rawCnt[m];
			}
			b[c++] = p;
			p = 0;
		}
		
		if (matches[i].off <= SHRTOFF && matches[i].len <= LOWMTCH) {
			// short offset
			// Tag: 01nxxxxx
			b[c++] = 0x40 | ((matches[i].len - MINMTCH) << 5) |
			(matches[i].off - 1);
		} else {
			// Tag: 1nnnnxxx + ([8]) + [8]
			b[c++] = matches[i].off-1;
			
			if (matches[i].len >= MIDMTCH) {
				m = matches[i].len-MIDMTCH;
				b[c++] = m;
			} else {
				m =  0;
			}
			
			// old b[c++] = 0x80 | ((matches[i].len - MINMTCH - m) << 2) |
			//((matches[i].off-1) >> 8);
			b[c++] = 0x80 | ((matches[i].len - MINMTCH - m) << 3) |
			((matches[i].off-1) >> 8);
		}
		i += matches[i].len;
	}
	
	// check remaining literals..	
	if (p > 0) {
		// Tag: 00nnnnnn
		for (m = 0; m < p; m++) {
		b[c++] = rawCnt[m];			}
		b[c++] = p;
	}
	
	return c;
}

//
//

static struct option longopts[] = {
	{"opt", no_argument, NULL, 'o'},
	{"non", no_argument, NULL, 'n'}
};

//

void usage( char *prg ) {
	fprintf(stderr,"Usage: %s [options] infile outfile\n",prg);
	fprintf(stderr," Options:\n"
				   "  --opt Selects optimal parsing\n"
				   "  --non Selects non-greedy matching (default)\n");
}
 
//
//

int main( int argc, char **argv ) {
	FILE *inf, *outf;
	int algo;
	int flen, elen;
	int n, m, p;
	u_int16_t hsh;
	int16_t nxt;

	fprintf(stderr, "ZXPac v0.1 - (c) 2009 Mr.Spiv of Scoopex\n");

	// set up stuff
	inf = NULL;
	outf = NULL;
	algo = ALGO_NON;
	
	//
	//

	while ((n = getopt_long(argc, argv, "on", longopts, NULL)) != -1) {
		switch (n) {
			case 'o':
				algo = ALGO_OPT;
				break;
			case 'n':
				algo = ALGO_NON;
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

	// Start compression..
	//
	
	initHash();

	if (flen < WINSIZE) {
		m = flen;
		elen = 0;
	} else {
		m = WINSIZE;
		elen = flen - WINSIZE;
	}

	for (p = n = 0; n < m; n++) {
		insertHash(p, calcHash(p));
		++p;
	}

	for (n = 0; n < flen; n++) {
		hsh = calcHash(n);
		nxt = deleteHash(hsh);
		getMatch(n, nxt, &matches[n], flen);

		if (n < elen) {
			insertHash(p,calcHash(p));
			++p;
		} else {
			insertHashDummy();
		}
	}

	if (algo == ALGO_NON) {
		n = 0;

		while (n < flen) {
			n = nonGreedy(n,elen);
		}
	} else {
		n = flen - 1;

		do {
			n = optimalParser(n, flen);
		} while (n >= 0);
	}

	// Encode literals + matches
	n = encode(0, flen, buf);

	// encode original length
	buf[n++] = flen;
	buf[n++] = flen >> 8;

	fprintf(stderr,"Crunched length: %d (%2.2f%%)\n",n,(float)(100.0 * (flen-n) / flen));

	flen = fwrite(buf,1,n,outf);

	if (flen != n) {
		fprintf(stderr,"** Error: fwrite() failed: %s\n", strerror(errno));
	}
	
	fclose(inf);
	fclose(outf);
	exit(EXIT_SUCCESS);
}
