/**
 * @file src/target_asc.cpp
 * @brief Handle ASCII target specific handling
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 * @copyright The Unlicense
 *
 *
 *
 */

#include <iomanip>
#include <iosfwd>
#include <fstream>

#include "target.h"


target_ascii::target_ascii(const targets::target* trg, const lz_config* cfg, std::ofstream& ofs) : target_base(trg,cfg,ofs)
{
}

target_ascii::~target_ascii(void)
{
}


int  target_ascii::preprocess(char* buf, int len)
{
    int n;

    if (m_cfg->verbose) {
        std::cout << "Checking for ASCII only content" << std::endl;
    }

    for (n = 0; n < len; n++) {
        if (buf[n] < 0) {
            std::cerr << "**Error (" << __FILE__ << ":" << __LINE__ << "): ASCII only file contains bytes greater than 0x7f\n";
            return -1;
        }
    }

    m_cfg->is_ascii = true;
    return len;
}

int  target_ascii::save_header(const char* buf, int len)
{
    (void)buf;
    (void)len;
    return 0;
}

int  target_ascii::post_save(int len)
{
    (void)len;
    return 0;
}

