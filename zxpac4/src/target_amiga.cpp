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

int amiga::preprocess(lz_config_t* cfg, char* buf, int len, void*& aux)
{
    std::vector<amiga_hunks::hunk_info_t> hunk_list(0);
    bool debug_on = cfg->debug_level > DEBUG_LEVEL_NONE ? true : false;
    char* amiga_exe = NULL;
    int n = amiga_hunks::parse_hunks(buf,len,hunk_list,debug_on);

    if (n <= 0) {
        std::cerr << ERR_PREAMBLE << "miga target hunk parsing failed" << std::endl;
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
            for (int m = 0; m < n; m++) {
                buf[m] = amiga_exe[m];
            }
        
            // Aling to 4 bytes and add room for the decompressor..
            // This code should take the compression algorithm into account..
            n = (n + AMIGA_EXE_DECOMPRESSOR_SIZE + AMIGA_EXE_SECURITY_DISTANCE + 3) & ~3;

            // we know that aux points at std::vector<uint32_t>.. so much for an abtract API ;)
            std::vector<uint32_t>* segs = reinterpret_cast<std::vector<uint32_t>*>(aux);
            
            // Fabricate a new hunk and insert the size of the compressed data + decompressor
            segs->push_back(n >> 2);
            segs->push_back(0xdeadbeef);
            segs->push_back(0xc0dedbad);
        }
        
        delete[] amiga_exe;
    }
    return n;
}

int amiga::save_header(const lz_config_t* cfg, char* buf, int len, std::ofstream& ofs, void* aux)
{
    int n;
    segment_size_t* segs = reinterpret_cast<segment_size_t*>(aux);
    int num_seg = segs->size() / 3;     // See src/hunk.cpp for content of the aux data 
    char* hdr;
    char* ptr;

    assert(segs->size() % 3 == 0);

    hdr = new (std::nothrow) char[sizeof(uint32_t) * (5 + num_seg + 3 * (num_seg+1))]; 
    
    if (hdr == NULL) {
        std::cerr << ERR_PREAMBLE << "memory allocation failed" << std::endl;
        return -1;
    }

    ptr = write32be(hdr,HUNK_HEADER);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,num_seg+1);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,num_seg);

    for (n = 0; n < num_seg; n++) {
        ptr = write32be(ptr,segs->at(3*n));    
    }
    
    // First hunk is always HUNK_CODE with 6 bytes of payload for a JSR
    ptr = write32be(ptr,segs->at(3*0 + 1));
    ptr = write32be(ptr,2);    
    
    // JSR 0x00000000   b0100111010111001
    ptr = write16be(ptr,0x4eb9);
    ptr = write32be(ptr,0x00000000);
    ptr = write16be(ptr,0x4e70);

    // Followed by a single relocation to the last hunk, which will contain
    // our decompressor and the compressed data
    ptr = write32be(ptr,HUNK_RELOC32);
    ptr = write32be(ptr,1);
    ptr = write32be(ptr,num_seg);
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
    ptr = write32be(ptr,0);

    // Write the decompressor..
    // TBD..

    // write it to the file
    n = ptr - hdr;

    ofs.write(hdr,n);
    if (!ofs) {
        std::cerr << ERR_PREAMBLE << "write failed (" << std::strerror(errno) << ")" << std::endl;
        n = -1;
    } else {
        // Preprare hunk for compressed data.. so that we can seek back to this position..
        // Store the pointer into the fabricated hunk data size location..
        segs->at(num_seg*3 - 1) = n;
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
    int n = len;

    if (n % 4) {
        // must align to 4..
        n = 4 - (n & 4);
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



