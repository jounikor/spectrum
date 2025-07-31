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
#include "hunk.h"

#define AMIGA_EXE_SECURITY_DISTANCE     8
#define AMIGA_OVERLAY_BUFFER_SIZE       2048

#define TRG_FALSE   0       // false
#define TRG_TRUE    1       // true
#define TRG_NSUP    -1      // feature not supported


namespace targets {
    
    /**
     * @struct target
     * @brief A structure to describe the target specific parts that are not
     *        compression algorithm specific.
     */
    struct target {
        const char* target_name;    /**< Target string. Currently supported targets:
                                         "asc" for 7bit ASCII.
                                         "bin" for raw binary data - no headers to be included.
                                         "zx"  for ZX Spectrum self-extracting TAP files.
                                         "bbc" for BBC Model A/B self-extracting files.
                                         "ami" for Amiga self-extracting exeutable files. */

        const char* target_brief;   /**< Brief description of the target. */
        int max_file_size;          /**< Maximum original file size for the target.
                                         zxpac4 algorithm limit is 2^24-1 bytes. */

        uint64_t supported_algorithms;  /**< A bit field of supported algoriths. These are indexed
                                          as 1<<ZXPAC4 etc.. */

        int algorithm;              /**< Default/selected algorithm used with this target:
                                          ZXPAC4      The 8bit optimized algorithm with 2^17-1 bit
                                                      sliding window.
                                          ZXPAC4B     A variation of ZXPAC4 with runlen literal
                                                      encoding. Number of uncompressible bytes is
                                                      limited to 255, though.
                                          ZXPAC4_32K  A variation of ZXPAC4 with maximum slidng
                                                      window of 2^15-1 bytes.
                                         There's a separeate data structure, which contains 
                                         default values for each alforithm. */
    
        int max_match;              /**< Maximum match length for this target. 0 mean use algorithm maximum. */
        uint32_t load_addr;         /**< Load address of the binary for a target. 0x0 if not used. */
        uint32_t jump_addr;         /**< Jump address to the binary for a target. 0x0 if not used. */
        const char* file_name;      /**< Filename for the output file e.g. ZX Spectrum TAP file. */

        int8_t initial_pmr;         /**< Target specific initial PMR offset. 0 use algorithm default. */
        int8_t overlay;             /**< Amiga target specific: use overlay decompressor. */
        int8_t merge_hunks;         /**< Amiga target specific: merge executable file hunks when possible. */
        int8_t equalize_hunks;      /**< Amiga target specific: treat HUNK_CODE/DATA/BSS the same. */
        int8_t encode_to_ram;       /**< Set TRUE if the comressed file is provided as a RAM buffer instead
                                         of directly saving into a file. */
    };

    struct decompressor {
        int length;
        uint8_t* code;
    };
}

/**
 * @class target_base
 * @brief A pure virtual base class for different targets. All targets must implement the
 *        API for preprocessing and saving the file. (De)Constructors may vary.
 */
class target_base {
protected:
    const targets::target* m_trg;
    const lz_config* m_cfg;
    std::ofstream& m_ofs;
    bool m_nohunks;
public:
    /**
     * @brief A constructor for all targets.
     * @param[in] trg A const ptr to targets::target with target specific data.
     * @param[in] cfg A ptr to lz_config for this file and target.
     * @param[in] ofs The ouput file stream.  
     */
    target_base(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs) : 
        m_trg(trg), m_cfg(cfg), m_ofs(ofs) {}
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
     *         Zero or negative value if there was an error.
     */
    virtual int preprocess(char* buf, int len) = 0;
    
    /**
     * @brief Save the new file header into the output file. The function may
     *        also modify the file buffer.
     *
     * @param buf[in]    A const ptr to the input buffer. The header saving
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
     *  The postamble function cannot change the content of the input file but
     *  may do other computation over the provided buffer. or modify the output
     *  file stream.
     * 
     * @param buf[in]    A ptr to compressed data. Can be NULL if the buffer 
     *                   is not available. This is useful e.g., in cases where the
     *                   post_save() function need to calculate a CRC over the
     *                   compressed data and saving the compressed data is done
     *                   in post_save() as well.
     *                   Note! this requires that the main loop passed a NULL ptr
     *                   instead of std::ofsteam to lz_encode(). In this case the
     *                   lz_encode() populates the input buffer with compressed
     *                   data.
     * @param len[in]    The length of the compressed file.
     *
     * @return The final size of the saved file. Negative value if there was
     *         an error.
     */
    virtual int post_save(const char* buf, int len) = 0;
};

class target_amiga : public target_base {
private:
    std::vector<amiga_hunks::new_hunk_info_t> m_new_hunks; 
    static const targets::decompressor exe_decompressors[]; 
    static const targets::decompressor abs_decompressors[]; 
    static const targets::decompressor exe_decompressors_255[]; 
    static const targets::decompressor abs_decompressors_255[]; 
    static const targets::decompressor overlay_decompressors[]; 

    int preprocess_exe(char* buf, int len);
    int preprocess_overlay(char* buf, int len);

    int save_header_exe(const char* buf, int len);
    int save_header_abs(const char* buf, int len);
    int save_header_overlay(const char* buf, int len);
    
    int post_save_exe(int len);
    int post_save_abs(int len);
    int post_save_overlay(int len);
public:
    target_amiga(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs);
    ~target_amiga(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(const char* buf, int len);
};

class target_ascii : public target_base {
public:
    target_ascii(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs);
    ~target_ascii(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(const char* buf, int len);
};

class target_binary : public target_base {
public:
    target_binary(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs);
    ~target_binary(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(const char* buf, int len);
};

class target_spectrum : public target_base {
    char tap_chksum(const char* b, char c, int n);
    char m_chksum;      /**< Partial checksum for data */

public:
    target_spectrum(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs);
    ~target_spectrum(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(const char* buf, int len);
};

class target_bbc : public target_base {
public:
    target_bbc(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs);
    ~target_bbc(void);
    int preprocess(char* buf, int len);
    int save_header(const char* buf, int len);
    int post_save(const char* buf, int len);
};


#endif // _TARGET_H_INCLUDED
