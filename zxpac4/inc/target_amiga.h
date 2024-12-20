/**
 * @file inc/target_amiga.h
 * @copyright The Unlicense
 *
 */
#ifndef _TARGET_AMIGA_H_INCLUDED
#define _TARGET_AMIGA_H_INCLUDED

#include <fstream>
#include "lz_base.h"

int preprocess_amiga(const lz_config_t* cfg, char* buf, int len, void*& aux);
int save_header_amiga(const lz_config_t* cfg, const char* buf, int len, std::ofstream& ofs, void* aux);
int post_save_amiga(const lz_config_t* cfg, const char* buf, int len, std::ofstream& ofs, void* aux);
void done_amiga(void* aux);

typedef std::vector<uint32_t> segment_size_t;


#endif  // _TARGET_AMIGA_H_INCLUDED
