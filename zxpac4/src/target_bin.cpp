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


int preamble_bin(lz_config_t* cfg, char*& buf, int& len, void*& aux)
{
    std::cerr << "Binary preamble_bin\n";

    return 0;
}

int postamble_bin(lz_config_t* cfg, char* buf, int len, std::ofstream& ofs, void* aux)
{
    std::cerr << "Binary postamble_bin\n";
    return 0;
}



