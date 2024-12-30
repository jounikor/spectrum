/**
 * @file inc/target_spectrum.h
 * @copyright The Unlicense
 *
 */
#ifndef _TARGET_SPECTRUM_H_INCLUDED
#define _TARGET_SPECTRUM_H_INCLUDED

#include <fstream>
#include "lz_base.h"

namespace spectrum {
    int preprocess(lz_config_t* cfg, char* buf, int len, void*& aux);
    int save_header(const lz_config_t* cfg, char* buf, int len, std::ofstream& ofs, void* aux);
    int post_save(const lz_config_t* cfg, int len, std::ofstream& ofs, void* aux);
    void done(void* aux);
};



#endif  // _TARGET_SPECTRUM_H_INCLUDED



