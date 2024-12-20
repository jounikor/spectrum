/**
 * @file src/target_amiga.cpp
 * @brief Amiga target specific executable file handling.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @copyright The Unlicense
 *
 *
 */

#include <vector>
#include "hunk.h"
#include "target_amiga.h"

using namespace amiga_hunks;


int preprocess_amiga(const lz_config_t* cfg, char* buf, int len, void*& aux)
{
    segment_size_t* hdr_segs = new segment_size_t;


    hdr_segs->resize(10);

    return 0;
}

int save_header_amiga(const lz_config_t* cfg, const char* buf, int len, std::ofstream& ofs, void* aux)
{
    int n;
    segment_size_t* segs = reinterpret_cast<segment_size_t*>(aux);
    int num_seg = segs->size() / 2;
    char* hdr;
    char* ptr;

    hdr = new char[sizeof(uint32_t) * (5 + num_seg + 3 * (num_seg+1))]; 
    ptr = write32be(hdr,HUNK_HEADER);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,num_seg+1);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,num_seg);

    for (n = 0; n < num_seg; n++) {
        ptr = write32be(ptr,segs->at(2*n));    
    }
    
    ptr = write32be(ptr,HUNK_CODE);
    ptr = write32be(ptr,segs->at(1));

    for (n = 1; n < num_seg; n++) {
        write32be(ptr,HUNK_BSS);
        ptr = write32be(ptr,segs->at(2*n + 1));    
    }



    delete[] hdr;

    return 0;
}

int post_save_amiga(const lz_config_t* cfg, const char* buf, int len, std::ofstream& ofs, void* aux)
{
    segment_size_t* seg = reinterpret_cast<segment_size_t*>(aux);
    return 0;
}

void done_amiga(void* aux)
{
    segment_size_t* seg = reinterpret_cast<segment_size_t*>(aux);

    if (seg) {
        delete seg;
    }
}



