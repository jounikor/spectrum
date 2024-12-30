/**
 * @file src/targte_bin.cpp
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
#include <fstream>

#include "lz_base.h"
#include "target_bin.h"

int  binary::preprocess(lz_config_t* cfg, char* buf, int len, void*& aux)
{
    std::cerr << "Binary preproces()\n";
    return len;
}

int  binary::save_header(const lz_config_t* cfg, char* buf, int len, std::ofstream& ofs, void* aux)
{
    std::cerr << "Binary save_header()\n";
    return 0;
}

int  binary::post_save(const lz_config_t* cfg, int len, std::ofstream& ofs, void* aux)
{
    std::cerr << "Binary post_save()\n";
    return len;
}

void binary::done(void* aux)
{
    std::cerr << "Binary done()\n";
    
    if (aux) {
        std::cerr << "Free AUX" << std::endl;
    }
}

