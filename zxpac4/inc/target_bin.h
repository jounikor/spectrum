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

namespace binary {
    int  preprocess(lz_config_t* cfg, char* buf, int len, void*& aux);
    int  save_header(const lz_config_t* cfg, char* buf, int len, std::ofstream& ofs, void* aux);
    int  post_save(const lz_config_t* cfg, int len, std::ofstream& ofs, void* aux);
    void done(void* aux);
};

#endif  // _TARGET_BIN_INCLUDED
