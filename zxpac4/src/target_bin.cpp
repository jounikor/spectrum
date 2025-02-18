/**
 * @file src/target_bin.cpp
 * @brief Handle binary target specific handling
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

target_binary::target_binary(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs) : target_base(trg,cfg,ofs) {
}

target_binary::~target_binary(void) {
}

int  target_binary::preprocess(char* buf, int len)
{
    (void)buf;
    return len;
}

int  target_binary::save_header(const char* buf, int len)
{
    (void)buf;
    (void)len;
    return 0;
}

int  target_binary::post_save(const char* buf, int len)
{
    (void)len;
    (void)buf;
    return len;
}

