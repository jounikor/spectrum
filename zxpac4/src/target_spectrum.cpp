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

target_spectrum::target_spectrum(const targets::target* trg, lz_config_t* cfg, std::ofstream& ofs) : target_base(trg,cfg,ofs) {
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
}

int  target_spectrum::post_save(int len)
{
    (void)len;
    return 0;
}

