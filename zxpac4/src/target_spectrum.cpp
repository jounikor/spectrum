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
    F9,C0
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
static const uint8_t tapLoader[] = {
	0x13,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x0A,0x37,0x00,
	0xF9,0xC0,0x28,0x28,0xBE,0xB0,0x22,0x32,0x33,0x36,0x33,0x35,0x22,0x2B,
	0xB0,0x22,0x32,0x35,0x36,0x22,0x2A,0xBE,0xB0,0x22,0x32,0x33,0x36,0x33,
	0x36,0x22,0x29,0x2B,0xB0,0x22,0x34,0x34,0x22,0x29,0x3A,0xEA
};


target_spectrum::target_spectrum(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs) : target_base(trg,cfg,ofs) {
    // Force reverse decompression.. these two are safe to modify here.
    cfg->reverse_file = true;
    cfg->reverse_encoded = true;
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
    int n = sizeof(tapLoader);

    m_ofs.write(reinterpret_cast<const char*>(&tapLoader[0]),sizeof(tapLoader));
    if (!m_ofs) {
        std::cerr << ERR_PREAMBLE << "write failed (" << std::strerror(errno) << ")" << std::endl;
        n = -1;
    }

    return n;
}

int  target_spectrum::post_save(int len)
{
    (void)len;
    int n;

    // Seek to the end end and return the final byte size
    m_ofs.seekp(0,std::ios_base::end);
    n = m_ofs.tellp();

    if (!m_ofs) {
        std::cerr << ERR_PREAMBLE << "post saving failed" << std::endl;
        n = -1;
    }

    return n;
}

