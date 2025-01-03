/**
 * @file inc/target.h
 * @brief A base class for target specific preprosessing and header saving.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 * @copyright The unlicense
 *
 *
 *
 */
#ifndef _TARGET_H_INCLUDED
#define _TARGET_H_INCLUDED

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <new>

#include "lz_util.h"
#include "lz_base.h"

namespace targets {
    struct target {
    // Target string. Currently supported:
        const char* target_name;    /**< Target string. Currently supported:
                                         "asc" for 7bit ASCII
                                         "bin" for raw binary data - no headers to be included
                                         "zx"  for ZX Spectrum self-extracting TAP files
                                         "bbc" for BBC Model A/B self-extracting files
                                         "ami" for Amiga self-extracting exeutable files */

        int max_file_size;          /**< Maximum original file size for the target..
                                         zxpac4 algorithm limit is 2^24-1 bytes */

        uint64_t supported_algorithms;  /**< A bit field of supported algoriths. These are indexed
                                          as 1<<ZXPAC4 etx.. */

        int algorithm;              /**< Default algorithm used with this target. Currently supported:
                                          ZXPAC4      The 8bit optimized algorithm with 2^17-1 bit
                                                      sliding window.
                                          ZXPAC4B     A variation of ZXPAC4 with runlen literal
                                                      encoding. Number of uncompressible bytes is
                                                      limited to 255, though.
                                          ZXPAC4_32K  A variation of ZXPAC4 with maximum slidng
                                                      window of 2^15-1 bytes.
                                         There's a separeate data structure, which contains 
                                         default values for each alforithm. */
    
        uint32_t load_addr;         /**< Load address of the binary for a target. 0x0 if not used */
        uint32_t jump_addr;         /**< Jumo address to the binary for a target. 0x0 if not used */

        bool overlay:1;             /**< Amiga specific: use overlay decompressor. */
        bool merge_hunks:1;         /**< Amiga specific: merge executable file hunks when possible. */
    };
}


class target_base {
protected:
    lz_config* m_cfg;
    std::ofstream& m_ofs;
public:
    /**
     * @brief A constructor for all targets.
     * @param cfg[in]    A ptr to lz_config for this file and target.
     * @param ofs[in]    The ouput file stream.  
     */
    target_base(lz_config_t* cfg, std::ofstream& ofs) : m_cfg(cfg), m_ofs(ofs) {}
    virtual ~target_base(void) {}

    /**
     * @brief Do preprocessing to the input file. 
     *
     *  The preprocessor may change the content of the input file. The
     *  retuned buffer length may be shorted or equal to the original
     *  buffer size but never longer.
     * 
     * @param buf[inout] A ptr to the input buffer. The preprocessor
     *                   may change the content of the buffer.
     * @param len[in]    The length of the input buffer.
     *
     * @return New length of the input buffer afther preprocessing.
     *         Negative value if there was an error.
     */
    virtual int preprocess(char* buf, int len) = 0;
    
    /**
     * @brief Save the new file header into the output file. The function may
     *        also modify the file buffer.
     *
     * @param buf[in]    A ptr to the input buffer. The header saving
     *                   must not change the content of the buffer.
     * @param len[in]    The length of the input buffer.
     *
     * @return The number of saved bytes. Negative value if there was
     *         an error.
     */
    virtual int save_header(const char* buf, int len) = 0;
    
    /**
     * @brief Save a possible postamble or fix the header in the output file.
     *
     *  The postamble function may change the content of the input file. The
     *  retuned buffer length may be longer than the original buffer size.
     * 
     * @param len[in]    The length of the compressed file.
     *
     * @return The final size of the saved file. Negative value if there was
     *         an error.
     */
    virtual int post_save(int len) = 0;
};

// there shall be a separate implementation for each target..


class target_amiga : public target_base {
    std::vector<uint32_t> m_new_hunks; 
    uint32_t m_load;
    uint32_t m_jump;
    bool m_overlay:1;
    bool m_merge_hunks:1;
public:
    target_amiga(const targets::target* trg, lz_config_t* cfg, std::ofstream& ofs);
    ~target_amiga(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(int len);
};

class target_ascii : public target_base {
public:
    target_ascii(lz_config_t* cfg, std::ofstream& ofs);
    ~target_ascii(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(int len);
};

class target_binary : public target_base {
public:
    target_binary(lz_config_t* cfg, std::ofstream& ofs);
    ~target_binary(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(int len);
};

class target_spectrum : public target_base {
public:
    target_spectrum(lz_config_t* cfg, std::ofstream& ofs);
    ~target_spectrum(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(int len);
};

class target_bbc : public target_base {
public:
    target_bbc(lz_config_t* cfg, std::ofstream& ofs);
    ~target_bbc(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(int len);
};


#endif // _TARGET_H_INCLUDED
