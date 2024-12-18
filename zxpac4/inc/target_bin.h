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

int  preprocess_bin(const lz_config_t* cfg, char* buf, int len, void*& aux);
int  save_header_bin(const lz_config_t* cfg, const char* buf, int len, std::ofstream& ofs, void* aux);
int  post_save_bin(const lz_config_t* cfg, const char* buf, int len, std::ofstream& ofs, void* aux);
void done_bin(void* aux);

#endif  // _TARGET_BIN_INCLUDED
