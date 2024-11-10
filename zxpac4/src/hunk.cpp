/**
 * @file hunk.cpp
 * @brief Amiga executable hunk parsing and processing.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1*
 *
 * @copyright The Unlicense
 *
 */

#include "hunk.h"

using namespace amiga_hunks;

static const char* s_hunk_str[] = {
    "HUNK_UNIT",	            // 0x3E7       // error
    "HUNK_NAME",                // 0x3E8       // exe remove
    "HUNK_CODE",	            // 0x3E9       // exe -> 00
    "HUNK_DATA",	            // 0x3EA       // exe -> 01
    "HUNK_BSS",	                // 0x3EB       // exe -> 10
    "HUNK_RELOC32",	            // 0x3EC       // exe -> 11
    "HUNK_RELOC16",	            // 0x3ED       // error
    "HUNK_RELOC8",	            // 0x3EE       // error
    "HUNK_EXT",		            // 0x3EF       // error
    "HUNK_SYMBOL",		        // 0x3F0       // exe remove
    "HUNK_DEBUG",		        // 0x3F1       // exe remove
    "HUNK_END",		            // 0x3F2       // exe
    "HUNK_HEADER",		        // 0x3F3       // exe
    "Unknown_hunk",
    "HUNK_OVERLAY",	            // 0x3F5       // output exe / error
    "HUNK_BREAK",		        // 0x3F6       // output exe / error
    "HUNK_DREL32 or HUNK_RELOC32SHORT_OLD",		        // 0x3F7       // error
    "HUNK_DREL16",		        // 0x3F8       // error
    "HUNK_DREL8",		        // 0x3F9       // error
    "HUNK_LIB",		            // 0x3FA       // error
    "HUNK_INDEX",		        // 0x3FB       // error
    "HUNK_RELOC32SHORT",	    // 0x3FC       // exe -> 11
    "HUNK_RELRELOC32",	        // 0x3FD       // exe
    "HUNK_ABSRELOC16"	        // 0x3FE       // error
};

uint32_t amiga_hunks::read32be(const uint32_t* ptr)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(ptr);
    uint32_t r;

    r  = *p++; r <<= 8;
    r |= *p++; r <<= 8;
    r |= *p++; r <<= 8;
    r |= *p++;
    return r;
}

uint16_t amiga_hunks::read16be(const uint16_t* ptr)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(ptr);
    uint16_t r;

    r  = *p++; r <<= 8;
    r |= *p++;
    return r;
}



int amiga_hunks::parse_hunks(const char* buf, int size, hunk_info_t*& hunk_list, bool verbose)
{
    uint32_t* ptr = (uint32_t*)buf;
    uint32_t* end = (uint32_t*)(buf+size);
    uint32_t n, m, j;
    uint32_t tsize, tnum, tmax;     // Follow DOS RKRM ;)
    uint32_t msj, mtj;
    uint32_t num_residents;              // archaic stuff
    uint32_t hunk_type, hunk_size;
    uint16_t* c;
    int padding;

    if (size % 4) {
        std::cerr << "**Error: file size must be 4 byte aligned." << std::endl;
        return -1;
    }
    
    hunk_type = read32be(ptr++);

    if (size < 6*4 || (hunk_type != HUNK_HEADER)) { 
        std::cerr << "**Error: not Amiga executable." << std::endl;
        return -1;
    }
    if (verbose) {
        std::cout << "HUNK_HEADER (0x" << std::hex << hunk_type << ")" << std::endl;
    }

    num_residents = 0;
    while ((n = read32be(ptr++))) {
        ptr += n;
        ++num_residents;
    }

    if (num_residents > 0 && verbose) {
        std::cout << "  File has " << num_residents << " resident libraries.." << std::endl;
    }

    tsize = read32be(ptr++);
    tnum  = read32be(ptr++);
    tmax  = read32be(ptr++);
   
    if (verbose) {
        std::cout << "  Number of hunk: " << tsize << std::endl;
        std::cout << "  First hunk: " << tnum << std::endl;
        std::cout << "  Last of hunk: " << tmax << std::endl;
    }
    if (num_residents != tnum) {
        std::cerr << "  First root hunk (" << tnum << ") does not match the number of residents" << std::endl;
        return -1;
    }
    if ((tsize - 1 > tmax) && verbose) {
        std::cout << "  Likely an OVERLAY file" << std::endl;
    }

    hunk_list = new hunk_info_t[tmax-tnum+1];

    for (n = tnum, m = 0; n <= tmax; n++, m++) {
        msj = read32be(ptr++);
        // MEMF_PUBLIC == 1
        // MEMF_CHIP   == 2
        // MEMF_FAST   == 4
        // msj bit 31  == FAST
        // msj bit 30  == CHIP
        mtj = (msj & 0xc0000000) >> 29;
        msj = (msj & 0x3fffffff) << 2;

        if (mtj == (3 << 1)) {
            mtj = read32be(ptr++);
        }
        if (verbose) {
            std::cout << std::dec << "  Segment: " << n << " memory size is " << msj << " bytes "
                << "of memory type " << mtj << std::endl;
        }

        hunk_list[m].hunk_num = n;
        hunk_list[m].memory_size = msj;
        hunk_list[m].memory_type = mtj;
    }

    hunk_size = 0;
    n = -1;

    while (ptr < end) {
        hunk_size = 0;
        msj = read32be(ptr++);
        // Bits 31 and 30 are unused
        // Bit 29 contains flag for possible hunk advisory..
        hunk_type = msj & 0x1fffffff;
        
        if (verbose && hunk_type <= LAST_SUPPORTED_HUNK) {
            std::cout << s_hunk_str[hunk_type-FIRST_SUPPORTED_HUNK] << "(0x" << std::hex 
                << hunk_type << ")" << std::endl;
        } 
        // skip memory type but parse advisory hunk flag
        if (msj & 0x20000000) {
            hunk_size = read32be(ptr++);

            if (verbose) {
                std::cout << "  Advisory hunk flag seen.. skipping " << hunk_size
                    << " long words in hunk " << tnum << std::endl;
                ptr += hunk_size;
            }
        }

        switch (hunk_type) {
        case HUNK_SYMBOL:
            if (verbose) {
                std::cout << "  Removing.." << std::endl;
            }
            do {
                hunk_size = read32be(ptr++) & 0x00ffffff;
                if (hunk_size == 0) {
                    break;
                }
                ptr += (hunk_size + 1);
            } while (true);
            break;
        case HUNK_DEBUG:
        case HUNK_NAME:
            if (verbose) {
                std::cout << "  Removing.." << std::endl;
            }
            hunk_size = read32be(ptr++);
            break;
        case HUNK_DATA:
        case HUNK_CODE:
            hunk_size = read32be(ptr++);
        case HUNK_BSS:
            // we increment to next segment due exe files that may not
            // contain HUNK_ENDs
            ++n;
            hunk_list[n].segment_start = ptr;
            hunk_list[n].data_size = hunk_size << 2;
            if (verbose) {
                std::cout << std::dec << "  segment: " << hunk_list[n].hunk_num 
                    << " with data size: " << hunk_list[n].data_size
                    << " and memory size: " << hunk_list[n].memory_size << std::endl;
            }
            break;
        case HUNK_END:
            hunk_size = 0;
            break;
        case HUNK_RELOC32:
        case HUNK_RELRELOC32:
            hunk_list[n].reloc_start = ptr;
            hunk_list[n].short_reloc = false;
            hunk_size = 0;
            do {
                m = read32be(ptr++);
                if (m == 0) {
                    break;
                }
                // Skip "to which hunk relocation is to"
                j = read32be(ptr++);
                if (verbose) {
                    std::cout << std::dec << "  " << m << " relocs to segment " << j << std::endl;
                }
                // Skip reloc entries
                ptr += m;
            } while (true);
            break;
        case HUNK_DREL32:
        case HUNK_RELOC32SHORT:
            padding = 1;
            c = reinterpret_cast<uint16_t*>(ptr);
            hunk_list[n].reloc_start = ptr;
            hunk_list[n].short_reloc = true;
            hunk_size = 0;
            do {
                m = read16be(c++);
                if (m == 0) {
                    break;
                }
                // Skip "to which hunk relocation is to"
                j = read16be(c++);
                if (verbose) {
                    std::cout << std::dec << "  " << m << " relocs to segment " << j << std::endl;
                }
                // Skip reloc entries
                c += m;
                padding += m;
            } while (true);
            if (padding & 1) {
                ++c;
            }
            ptr = reinterpret_cast<uint32_t*>(c);
            break;
        case HUNK_BREAK:
        case HUNK_OVERLAY:
        default:
            std::cerr << "**Error: unsupported hunk 0x" << std::hex << hunk_type << std::endl;
            delete[] hunk_list;
            return -1;
        }

        ptr += hunk_size;
    }

    if (n+1 < tsize && verbose) {
        std::cout << "Number of parsed segments does not match with the header information.." << std::endl;
    }
    return n+1;
}


void amiga_hunks::free_hunk_info(hunk_info_t* hunk_list)
{
    delete[] hunk_list;
}

