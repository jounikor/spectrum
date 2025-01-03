/**
 * @file src/targte_asc.cpp
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

#include "lz_base.h"
#include "target_asc.h"

int  ascii::preprocess(lz_config_t* cfg, char* buf, int len, void*& aux)
{
    (void)aux;
    int n;

    if (cfg->verbose) {
        std::cout << "Checking for ASCII only content" << std::endl;
    }

    for (n = 0; n < len; n++) {
        if (buf[n] < 0) {
            std::cerr << "**Error (" << __FILE__ << ":" << __LINE__ << "): ASCII only file contains bytes greater than 127\n";
            return -1;
        }
    }

    cfg->is_ascii = true;
    return len;
}

int  ascii::save_header(const lz_config_t* cfg, char* buf, int len, std::ofstream& ofs, void* aux)
{
    (void)cfg;
    (void)buf;
    (void)len;
    (void)ofs;
    (void)aux;
    return len;
}

int  ascii::post_save(const lz_config_t* cfg, int len, std::ofstream& ofs, void* aux)
{
    (void)cfg;
    (void)len;
    (void)ofs;
    (void)aux;
    return len;
}

void ascii::done(void* aux)
{
    (void)aux;
}


