/**
 * @file src/target_amiga.cpp
 * @brief Amiga target specific executable file handling.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @copyright The Unlicense
 *
 *
 */

#include <vector>
#include <cerrno>
#include <cstring>
#include <iostream>
#include "hunk.h"
#include "target_amiga.h"
#include "lz_util.h"

using namespace amiga_hunks;

// First 4 bytes is always reserved for the final compressed size of the file
static uint8_t dec1[] = {0xff,0xff,0xff,0xff,0xff,0x60,0x70,0x80,0x90,0x00,0xff,0xff}; 
static uint8_t dec2[] = {0xff,0xff,0xff,0xff,0xff,0x61,0x71,0x81,0x91,0x01,
                         0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81,0x91,0x01}; 
static uint8_t dec3[] = {0xff,0xff,0xff,0xff,0xff,0x62,0x72,0x82,0x92,0x02,
                         0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82,0x92,0x02,
                         0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82,0x92,0x02,
                         0xff,0xff}; 

// Length must be even modulo 4
static amiga::decompressor decompressors[] = {
    {12,dec1,
    },
    {20,dec2,
    },
    {32,dec3
    }
};



int amiga::preprocess(lz_config_t* cfg, char* buf, int len, void*& aux)
{
    std::vector<amiga_hunks::hunk_info_t> hunk_list(0);
    bool debug_on = cfg->debug_level > DEBUG_LEVEL_NONE ? true : false;
    char* amiga_exe = NULL;
    int n = amiga_hunks::parse_hunks(buf,len,hunk_list,debug_on);
    int memory_len, m;

    if (n <= 0) {
        std::cerr << ERR_PREAMBLE << "Amiga target hunk parsing failed (" << n << " <= " << len  << std::endl;
    } else {
        std::vector<uint32_t>* new_hunks = new (std::nothrow) std::vector<uint32_t>;
    
        if (new_hunks == NULL) {
            std::cerr << ERR_PREAMBLE << "failed for allocated memory for Amiga target" << std::endl;
            return -1;
        }
        if (cfg->merge_hunks) {
            n = amiga_hunks::merge_hunks(buf,len,hunk_list,amiga_exe,new_hunks,debug_on);
        } else {
            n = amiga_hunks::optimize_hunks(buf,len,hunk_list,amiga_exe,new_hunks,debug_on);
        }
        if (n < 0 || n > len) {
            std::cerr << ERR_PREAMBLE << "Amiga target hunk preprocessing failed" << std::endl;
            delete[] new_hunks;
            n = -1;
        } else {
            aux = reinterpret_cast<void*>(new_hunks);
            for (m = 0; m < n; m++) {
                buf[m] = amiga_exe[m];
            }
        
            // Add decompressor size + some security length
            memory_len = n + decompressors[cfg->algorithm].length + AMIGA_EXE_SECURITY_DISTANCE;

            // Fabricate a new hunk and insert the size of the compressed data
            new_hunks->push_back((memory_len+3) >> 2);
            new_hunks->push_back(0xdeadbeef);
            new_hunks->push_back(0xc0dedbad);
        }
        
        delete[] amiga_exe;
    }
    return n;
}

int amiga::save_header(const lz_config_t* cfg, char* buf, int len, std::ofstream& ofs, void* aux)
{
    int n;
    std::vector<uint32_t>* segs = reinterpret_cast<std::vector<uint32_t>*>(aux);
    int num_seg = segs->size() / 3;     // See src/hunk.cpp for content of the aux data 
    char* hdr;
    char* ptr;

    assert(segs->size() % 3 == 0);
    hdr = new (std::nothrow) char[sizeof(uint32_t) * (5 + num_seg + 10
        + (3 * num_seg)) + decompressors[cfg->algorithm].length + 4]; 
    
    if (hdr == NULL) {
        std::cerr << ERR_PREAMBLE << "memory allocation failed" << std::endl;
        return -1;
    }

    ptr = write32be(hdr,HUNK_HEADER);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,num_seg);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,num_seg-1);

    for (n = 0; n < num_seg; n++) {
        ptr = write32be(ptr,segs->at(3*n));    
    }
    
    // First hunk is always HUNK_CODE with 6 bytes of payload for a JSR
    ptr = write32be(ptr,segs->at(3*0 + 1));
    ptr = write32be(ptr,2);    
    
    // JSR 0x00000000   b0100111010111001
    ptr = write16be(ptr,0x4eb9);        // JSR
    ptr = write32be(ptr,0x00000004);    // to 4th byte of the last segment
    ptr = write16be(ptr,0x4e70);        // RESET ;)

    // Followed by a single relocation to the last hunk, which will contain
    // our decompressor and the compressed data
    ptr = write32be(ptr,HUNK_RELOC32);
    ptr = write32be(ptr,1);
    ptr = write32be(ptr,num_seg-1);
    ptr = write32be(ptr,2);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,HUNK_END);
    
    // Write rest of the hunks minus the last (with compressed data) with data size of 0..
    for (n = 1; n < num_seg-1; n++) {
        ptr = write32be(ptr,segs->at(3*n + 1));
        ptr = write32be(ptr,0x00000000);
        ptr = write32be(ptr,HUNK_END);
    }

    // the last HUNK_CODE to host the compressed file. We do not know its data size yet..
    ptr = write32be(ptr,HUNK_CODE);
    
    // record the location for patching the data size.. for now put 0 there.
    n = ptr - hdr;
    segs->at(num_seg*3 - 1) = n;
    ptr = write32be(ptr,0);

    // Write the decompressor..
    for (n = 0; n < decompressors[cfg->algorithm].length; n++) {
        *ptr++ = decompressors[cfg->algorithm].code[n];
    }

    // write it to the file
    n = ptr - hdr;
    ofs.write(hdr,n);
    if (!ofs) {
        std::cerr << ERR_PREAMBLE << "write failed (" << std::strerror(errno) << ")" << std::endl;
        n = -1;
    }

    delete[] hdr;
    return n;
}


//
//
//
//

int amiga::post_save(const lz_config_t* cfg, int len, std::ofstream& ofs, void* aux)
{
    std::vector<uint32_t>* segs = reinterpret_cast<std::vector<uint32_t>*>(aux);
    char tmp[4] = {0};
    int original_len = len;
    int n;

    len += decompressors[cfg->algorithm].length;

    if (len % 4) {
        // must align to 4..
        n = 4 - (len % 4);
        ofs.write(tmp,n);
        len += n;
    }

    // Write HUNK_END
    write32be(tmp,HUNK_END,false);
    ofs.write(tmp,4);

    // Patch the HUNK_CODE size with the compressed data size in words..
    n = segs->size();
    ofs.seekp(segs->at(n-1),std::ios_base::beg);
    write32be(tmp,len>>2,false);
    ofs.write(tmp,4);

    // Patch the decompressor with the original byte size..
    write32be(tmp,original_len,false);
    ofs.write(tmp,4);

    // Seek to the end end and return the final byte size
    ofs.seekp(0,std::ios_base::end);
    n = ofs.tellp();

    if (!ofs) {
        std::cerr << ERR_PREAMBLE << "post saving failed" << std::endl;
        n = -1;
    }

    return n;
}

void amiga::done(void* aux)
{
    segment_size_t* seg = reinterpret_cast<segment_size_t*>(aux);

    if (seg) {
        delete seg;
    }
}



