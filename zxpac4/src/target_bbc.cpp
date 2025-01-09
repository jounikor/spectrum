/**
 * @file src/target_bbc.cpp
 * @brief Handle ASCII target specific handling
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 * @copyright The Unlicense
 *
 *
 *
 */

#include "target.h"

target_bbc::target_bbc(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs) : target_base(trg,cfg,ofs) {
}

target_bbc::~target_bbc(void) {
}


int  target_bbc::preprocess(char* buf, int len)
{
    (void)buf;
    return len;
}

int  target_bbc::save_header(const char* buf, int len)
{
    (void)buf;
    (void)len;
    return 0;
}

int  target_bbc::post_save(int len)
{
    (void)len;
    return len;
}

