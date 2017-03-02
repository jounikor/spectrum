///////////////////////////////////////////////////////////////////////////////
//
// Scoopex ZX Spectrum Cruncher TAP file creator v0.1 - ZXTap
// (c) 2009 Jouni 'Mr.Spiv' Korhonen
// This code is for public domain (if you so insane to use it..), so don't
// complain about my messy one file implementation.
//
// Originally coded for the 'ScoopZX' intro. See the companion
// cruncher tool ZXPac. The TAP "spectrum exe-packer" tool is purposely
// separate. The ZXTap tool allows creating "auto running" crunched one file
// programs. This is well suited for demos or games but not really for anything
// more advanced.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <errno.h>

/* The TAP file format..

13 00
00			// flag: 00 is header
00			// type: BASIC block
20 20 20 20 20 20 20 20 20 20	// file name
4B 00		// basic size		// 14-15
00 00		// autorun line 0
4B 00		// variable area	// 18-19
00			// checksum			// 20

4D 00		// length			// 21-22
FF			// data
00,0A		// Line number
37,00		// length			// 26-27
 ,F9,C0
28,28,BE,B0,22,32,33,36
33,35,22,2B,B0,22,32,35
36,22,2A,BE,B0,22,32,33
36,33,36,22,29,2B,B0,22
34,34		// offset
22,29,3A
EA			// REM
// data starts here -> offset 44
0D 
xx			// checksum

*/

// The hex code below contains to the following BASIC program:
//
//   10 RANDOMIZE USR ((PEEK VAL "23635"+VAL "256"*PEEK
// VAL "23636")+VAL "44"): REM 
//
// Do not edit or change the hexdump unless you really know what you
// are going to do.
static u_int8_t tapLoader[] = {
	0x13,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x0A,0x37,0x00,
	0xF9,0xC0,0x28,0x28,0xBE,0xB0,0x22,0x32,0x33,0x36,0x33,0x35,0x22,0x2B,
	0xB0,0x22,0x32,0x35,0x36,0x22,0x2A,0xBE,0xB0,0x22,0x32,0x33,0x36,0x33,
	0x36,0x22,0x29,0x2B,0xB0,0x22,0x34,0x34,0x22,0x29,0x3A,0xEA
};

#define TAPLOADERSIZE sizeof(tapLoader)
#define BASICSIZE 44+1			// terminating 0x0d
#define BASICLOADERSIZE 40+1	// includes terminating 0x0d

// Surprise! Max 64K files are supported..
static u_int8_t buf[65536];

//
// Calculate TAP checksums..

u_int8_t chksum( u_int8_t *b, int n ) {
	u_int8_t c = 0;
	int i;
	
	for (i = 0; i < n; i++) {
		c ^= b[i];
	}
	
	return c;	
}

//
// Patch in TAP + BASIC specific information..

int basicLoader( u_int8_t *tap, int offset, int dataLen, char *name ) {
	u_int16_t basicLen;

	// copy basic file name
	memset(tap+4,0x20,10);
	memcpy(tap+4,name,strlen(name) < 10 ? strlen(name) : 10);

	// basic program length
	basicLen = BASICSIZE + dataLen;
	tap[14] = basicLen;
	tap[15] = basicLen >> 8;
	tap[18] = tap[14];
	tap[19] = tap[15];

	// checksum for the basic header..
	tap[20] = chksum(tap+2,18);

	// basic program length.. in tap
	tap[21] = (basicLen+2);	// includes flag + checksum
	tap[22] = (basicLen+2) >> 8;

	// basic program length in our "basic" program
	tap[26] = (BASICLOADERSIZE+dataLen);
	tap[27] = (BASICLOADERSIZE+dataLen) >> 8;

	// add 0x0d to the end of our "basic" program
	tap[TAPLOADERSIZE+dataLen] = 0x0d;
	
	// checksum
	tap[TAPLOADERSIZE+dataLen+1] = chksum(tap+23,basicLen+2);

	// return also 0x0d + checksum..
	return TAPLOADERSIZE+dataLen+2;
}

//
//

void usage( char *prg ) {
	fprintf(stderr,"Usage: %s infile outfile [program name]\n",prg);
}


//
//

int main( int argc, char **argv ) {
	int dataLen, pos;
	int tapLen;
	FILE *inf, *outf;
	
	//
	if (argc < 3) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	
	//
	// Open files..
	inf = fopen(argv[1], "r+b");
	outf = fopen(argv[2], "wb");
	
	if (!(inf && outf)) {
		if (inf) { fclose(inf); }
		if (outf) { fclose(outf); }
		fprintf(stderr, "**Error: fopen() failed: %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	// Use global output buffer 'buf' and copy the TAP header..
	memcpy(buf,tapLoader,TAPLOADERSIZE);
	pos = TAPLOADERSIZE;

	// Read in the binary to be launched..
	if ((dataLen = fread(buf+pos,1,65536,inf)) < 0) {
		fprintf(stderr, "**Error: fread() failed: %s\n",strerror(errno));
		fclose(inf);
		fclose(outf);
		exit(EXIT_FAILURE);
	}

	pos += dataLen;

	// ..and write the final TAP..
	// In a case program name is not given use the output file name.
	tapLen = basicLoader(buf,TAPLOADERSIZE,dataLen,argc < 4 ? argv[2] : argv[3]);
	if (fwrite(buf,1,tapLen,outf) != tapLen) {
		fprintf(stderr, "**Error: fwrite() failed: %s\n",strerror(errno));
		fclose(inf);
		fclose(outf);
		exit(EXIT_FAILURE);
	}

	// Close stuff..
	fclose(inf);
	fclose(outf);
	
	
	return 0;
}






