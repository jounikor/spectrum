/**
 * @file inc/target_amiga.h
 * @copyright The Unlicense
 *
 */
#ifndef _TARGET_AMIGA_H_INCLUDED
#define _TARGET_AMIGA_H_INCLUDED

#include <fstream>
#include "lz_base.h"

namespace amiga {
    int preprocess(lz_config_t* cfg, char* buf, int len, void*& aux);
    int save_header(const lz_config_t* cfg, char* buf, int len, std::ofstream& ofs, void* aux);
    int post_save(const lz_config_t* cfg, int len, std::ofstream& ofs, void* aux);
    void done(void* aux);
};

typedef std::vector<uint32_t> segment_size_t;


#define AMIGA_EXE_DECOMPRESSOR_SIZE 512
#define AMIGA_EXE_SECURITY_DISTANCE 64


#endif  // _TARGET_AMIGA_H_INCLUDED
