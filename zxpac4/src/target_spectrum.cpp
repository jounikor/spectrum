/**
 * @file src/target_spectrum.cpp
 * @brief Handle ZX Spectrum target specific handling
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 * @copyright The Unlicense
 *
 *
 *
 */

#include <iomanip>
#include <iosfwd>
#include "target.h"

/* The TAP file format..

    HEADER BLOCK
    13 00
    <----1
    00			// flag: 00 is header
    00			// type: BASIC block
    20 20 20 20 20 20 20 20 20 20	// file name
    4B 00		// basic size		// 14-15
    00 00		// autorun line 0
    4B 00		// variable area	// 18-19
    1----> first cehcksum covers this area; 18 bytes
    00			// checksum			// 20

    CODE BLOCK
    4D 00		// length			// 21-22
    <----2
    FF			// data flag        // 23 -> checksum for data starts here
    <----3
    00,0A		// Line number
    37,00		// length			// 26-27
    F9,C0                           // 28-29
    28,28,BE,B0,22,32,33,36         // 30-38
    33,35,22,2B,B0,22,32,35         // 39-46
    36,22,2A,BE,B0,22,32,33         // 47-54
    36,33,36,22,29,2B,B0,22         // 55-62
    34,34		// offset           // 63-64
    22,29,3A                        // 65-67
    EA			// REM              // 68
    3----> TAPBASICSIZE-1
    // data starts here -> offset 68
    2----> second checksum
    xx			// checksum         // 68+length
*/

// The hex code below contains to the following BASIC program:
//
//   10 RANDOMIZE USR ((PEEK VAL "23635"+VAL "256"*PEEK
//      VAL "23636")+VAL "44"): REM ...
//
// Do not edit or change the hexdump unless you really know what you
// are going to do.
static const uint8_t tapLoader[] = {
	0x13,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x0A,0x37,0x00,
	0xF9,0xC0,0x28,0x28,0xBE,0xB0,0x22,0x32,0x33,0x36,0x33,0x35,0x22,0x2B,
	0xB0,0x22,0x32,0x35,0x36,0x22,0x2A,0xBE,0xB0,0x22,0x32,0x33,0x36,0x33,
	0x36,0x22,0x29,0x2B,0xB0,0x22,0x34,0x34,0x22,0x29,0x3A,0xEA,
};

#include "../z80/z80tap_255_32k.h"

#define Z80DECSIZE          sizeof(z80tap_255_32k_bin)
#define TAPLOADERSIZE       sizeof(tapLoader)
#define TAPBASICSIZE        44+0			            // terminating 0x0d excluded
#define TAPBASICLOADERSIZE  40+0	                    // excludes terminating 0x0d
#define Z80LOADADDR         TAPLOADERSIZE+8             // TAP to start of decompressor + 8
#define Z80JUMPADDR         TAPLOADERSIZE+1            



char target_spectrum::tap_chksum(const char *b, char c, int n) {
	int i;
	
	for (i = 0; i < n; i++) {
		c ^= b[i];
	}
	
	return c;	
}

target_spectrum::target_spectrum(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs) : target_base(trg,cfg,ofs) {
    // Force reverse decompression.. these two are safe to modify here.
    cfg->reverse_file = true;
    cfg->reverse_encoded = true;
    m_chksum = 0;
}

target_spectrum::~target_spectrum(void) {
}


int  target_spectrum::preprocess(char* buf, int len)
{
    (void)buf;
    return len;
}

int  target_spectrum::save_header(const char* buf, int len)
{
    (void)buf;
    (void)len;
    return 0;
#if 0
    int n = TAPLOADERSIZE;

    // save TAP header and the BASIC program templete
    m_ofs.write(reinterpret_cast<const char*>(tapLoader),n);

    // save decompressor
    n = Z80DECSIZE;
    m_ofs.write(reinterpret_cast<const char*>(z80tap_255_32k_bin),n);

    if (!m_ofs) {
        std::cerr << ERR_PREAMBLE << "write failed (" << std::strerror(errno) << ")" << std::endl;
        n = -1;
    }

    return n;
#endif
}

int  target_spectrum::post_save(const char* buf, int len)
{
    char tap[TAPLOADERSIZE+Z80DECSIZE];
    int n;
    char c;

    // Copy the header and the decompressor
    ::memcpy(tap,tapLoader,TAPLOADERSIZE);
    ::memcpy(tap+TAPLOADERSIZE,z80tap_255_32k_bin,Z80DECSIZE);

    // patch name
    n = ::strlen(m_trg->file_name);
	::memcpy(tap+4,m_trg->file_name,n < 10 ? n : 10);

	// basic program length
    tap[14] = (TAPBASICSIZE+Z80DECSIZE+len);
	tap[15] = (TAPBASICSIZE+Z80DECSIZE+len) >> 8;
	tap[18] = tap[14];
	tap[19] = tap[15];

	// checksum for the basic header..
	tap[20] = tap_chksum(tap+2,0,18);

	// BASIC program length.. in tap
	tap[21] = (TAPBASICSIZE+Z80DECSIZE+len+2);	        // includes flag + checksum
	tap[22] = (TAPBASICSIZE+Z80DECSIZE+len+2) >> 8;

	// BASIC program length in our "BASIC" program
	tap[26] = (TAPBASICLOADERSIZE+Z80DECSIZE+len);     //
	tap[27] = (TAPBASICLOADERSIZE+Z80DECSIZE+len) >> 8;

    // Patch jump address
    tap[Z80JUMPADDR+0] = m_trg->jump_addr;
    tap[Z80JUMPADDR+1] = m_trg->jump_addr >> 8;

    // Patch load address
    tap[Z80LOADADDR+0] = m_trg->load_addr;
    tap[Z80LOADADDR+1] = m_trg->load_addr >> 8;

    // Save the patcher header..
    m_ofs.seekp(0,std::ios_base::beg);
    m_ofs.write(tap,TAPLOADERSIZE+Z80DECSIZE);

    // Save the compressed file
    m_ofs.write(buf,len);

	// checksum.. 
	c = tap_chksum(tap+23,0x00,TAPLOADERSIZE+Z80DECSIZE-23);
    c = tap_chksum(buf,c,len);
    
    // write checksum
    m_ofs.put(c);

    // return the final byte size
    //m_ofs.seekp(0,std::ios_base::end);
    n = m_ofs.tellp();

    if (!m_ofs) {
        std::cerr << ERR_PREAMBLE << "post saving failed" << std::endl;
        n = -1;
    }

    return n;
}

