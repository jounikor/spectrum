/**
 * @file inc/target_bin.h
 *
 *
 *
 *
 *
 */
#ifndef _TARGET_BIN_INCLUDED
#define _TARGET_BIN_INCLUDED

int preamble_bin(lz_config_t* cfg, char*& buf, int& len, void*& aux);
int postamble_bin(lz_config_t* cfg, char* buf, int len, std::ofstream& ofs, void* aux);

#endif  // _TARGET_BIN_INCLUDED
