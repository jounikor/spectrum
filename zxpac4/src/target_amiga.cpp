/**
 * @file src/target_amiga.cpp
 * @brief Amiga target specific executable file handling.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @copyright The Unlicense
 *
 *
 */

#include <vector>
#include <cerrno>
#include <cstring>
#include <iostream>
#include "hunk.h"
#include "target.h"



using namespace amiga_hunks;
using namespace targets;

// These include files are generated
#include "../m68k/zxpac4_exe.h"
#include "../m68k/zxpac4_exe_255.h"
#include "../m68k/zxpac4_exe_32k.h"
#include "../m68k/zxpac4_exe_255_32k.h"
#include "../m68k/zxpac4_abs.h"
#include "../m68k/zxpac4_abs_255.h"
#include "../m68k/zxpac4_abs_32k.h"
#include "../m68k/zxpac4_abs_255_32k.h"

// Length must be even
decompressor const target_amiga::exe_decompressors[] = { 
    {
        sizeof(zxpac4_exe_bin),
        zxpac4_exe_bin,
    },
    {
        32,
        NULL,   // This is still missing..
    },
    {
        sizeof(zxpac4_exe_32k_bin),
        zxpac4_exe_32k_bin,
    },
};

decompressor const target_amiga::exe_decompressors_255[] = { 
    {
        sizeof(zxpac4_exe_255_bin),
        zxpac4_exe_255_bin,
    },
    {
        32,
        NULL,   // This is still missing..
    },
    {
        sizeof(zxpac4_exe_255_32k_bin),
        zxpac4_exe_255_32k_bin,
    },
};

const decompressor target_amiga::abs_decompressors[] = { 
    {
        sizeof(zxpac4_abs_bin),
        zxpac4_abs_bin,
    },
    {
        sizeof(zxpac4_abs_32k_bin),
        zxpac4_abs_32k_bin,
    },
    {
        32,
        NULL,   // this is still missing..
    }
};

const decompressor target_amiga::abs_decompressors_255[] = { 
    {
        sizeof(zxpac4_abs_255_bin),
        zxpac4_abs_255_bin,
    },
    {
        sizeof(zxpac4_abs_255_32k_bin),
        zxpac4_abs_255_32k_bin,
    },
    {
        32,
        NULL,   // this is still missing..
    }
};

const decompressor target_amiga::overlay_decompressors[] = { 
    {
        sizeof(zxpac4_exe_bin),
        zxpac4_exe_bin,
    },
    {
        sizeof(zxpac4_exe_32k_bin),
        zxpac4_exe_32k_bin,
    },
    {
        32,
        NULL,   // This is still missing..
    }
};



target_amiga::target_amiga(const target* trg, const lz_config_t* cfg, std::ofstream& ofs) : target_base(trg,cfg,ofs) {
    // check target and config.. some parameter changes based settings
    // force reverse file if not an overlaid decompression used
    
    if (!trg->overlay) {
        if (cfg->verbose) {
            std::cout << "Enabling reverse file and encoding for Amiga target\n";
        }

        cfg->reverse_file = true;
        cfg->reverse_encoded = true;
    } else {
        if (cfg->verbose) {
            std::cout << "Disabling reverse file and encoding for Amiga target\n";
        }
        cfg->reverse_file = false;
        cfg->reverse_encoded = false;
    }
    if (trg->load_addr == 0 && trg->jump_addr == 0) {
        m_nohunks = false;
    } else {
        m_nohunks = true;
    }
}

target_amiga::~target_amiga(void) {
}

// Amiga target Preprocessing functions:
//  - normal  executable
//  - absolute address executable has no preprocessing
//  - overlay executable

int target_amiga::preprocess(char* buf, int len)
{
    m_new_hunks.clear();

    if (m_nohunks) {
        return len;
    }
    if (m_trg->overlay) {
        return preprocess_overlay(buf,len);
    }
    
    return preprocess_exe(buf,len);
}

int target_amiga::preprocess_exe(char* buf, int len)
{
    std::vector<amiga_hunks::hunk_info_t> hunk_list(0);
    bool debug_on = m_cfg->debug_level > DEBUG_LEVEL_NONE ? true : false;
    bool equalize = m_trg->equalize_hunks;
    char* amiga_exe = NULL;
    int m, n, memory_len;

    n = amiga_hunks::parse_hunks(buf,len,hunk_list,equalize,debug_on);
    
    if (n < 0) {
        std::cerr << ERR_PREAMBLE << "Amiga target hunk parsing failed (" << n << ")" << std::endl;
    } else {
        if (m_trg->merge_hunks) {
            n = amiga_hunks::merge_hunks(buf,len,hunk_list,amiga_exe,&m_new_hunks,debug_on);  
        } else {
            n = amiga_hunks::optimize_hunks(buf,len,hunk_list,amiga_exe,&m_new_hunks,debug_on);
        }
        if (n < 0 || n > len) {
            std::cerr << ERR_PREAMBLE << "Amiga target hunk preprocessing failed" << std::endl;
            n = -1;
        } else {
            for (m = 0; m < n; m++) {
                buf[m] = amiga_exe[m];
            }
        
            // Add decompressor size + some security length
            memory_len = n + exe_decompressors[m_cfg->algorithm].length + AMIGA_EXE_SECURITY_DISTANCE;

            // Fabricate a new hunk and insert the size of the compressed data
            new_hunk_info_t new_seg = {
                .mem_size_typed_longs = static_cast<uint32_t>((memory_len+3) >> 2),
                0xdeadbeef,
                0xc0dedbad
            };
            m_new_hunks.push_back(new_seg);
        }
        
        delete[] amiga_exe;
    }
    return n;
}


int target_amiga::preprocess_overlay(char* buf, int len)
{
    std::vector<amiga_hunks::hunk_info_t> hunk_list(0);
    bool debug_on = m_cfg->debug_level > DEBUG_LEVEL_NONE ? true : false;
    char* amiga_exe = NULL;
    int m, n, memory_len;
    int num_code_hunks = 0;
    new_hunk_info_t new_seg;

    n = amiga_hunks::parse_hunks(buf,len,hunk_list,debug_on);
    
    if (n < 0) {
        std::cerr << ERR_PREAMBLE << "Amiga target hunk parsing failed (" << n << ")" << std::endl;
        return -1;
    } 
    if (m_trg->merge_hunks) {
        n = amiga_hunks::merge_hunks(buf,len,hunk_list,amiga_exe,&m_new_hunks,debug_on);
    } else {
        n = amiga_hunks::optimize_hunks(buf,len,hunk_list,amiga_exe,&m_new_hunks,debug_on);
    }
    if (n < 0 || n > len) {
       std::cerr << ERR_PREAMBLE << "Amiga target hunk preprocessing failed" << std::endl;
       n = -1;
       goto overlay_error;
    }
    
    for (m = 0; m < static_cast<int>(m_new_hunks.size()); m++) {
        //if (m_new_hunks[m + 1] != HUNK_BSS) {
        if (m_new_hunks[m].hunk_type != HUNK_BSS) {
            if (++num_code_hunks > 1) {
                std::cerr << ERR_PREAMBLE << "only single code/data hunk allowed";
                goto overlay_error;
    }   }   }
    for (m = 0; m < n; m++) {
        buf[m] = amiga_exe[m];
    }
    
    // Add decompressor size + filebuffer size
    memory_len = overlay_decompressors[m_cfg->algorithm].length + AMIGA_OVERLAY_BUFFER_SIZE;

    // Fabricate a new hunk and insert the size of the compressed data
    new_seg = {
        .mem_size_typed_longs = static_cast<uint32_t>((memory_len+3) >> 2),
        0xdeadbeef,
        0xc0dedbad
    };
    m_new_hunks.push_back(new_seg);

overlay_error:
    delete[] amiga_exe;
    return n;
}

// Amiga target header saving functions:
//
//
//

int target_amiga::save_header(const char* buf, int len)
{
    if (m_nohunks) {
        return save_header_abs(buf,len);
    }
    if (m_trg->overlay) {
        return save_header_overlay(buf,len);
    }

    return save_header_exe(buf,len);
}

int target_amiga::save_header_abs(const char* buf, int len)
{
    (void)buf;
    (void)len;

    const decompressor* dec;
    int n;
    char* hdr;
    char* ptr;
    
    if (m_cfg->max_match < 256) {
        dec = &abs_decompressors_255[m_cfg->algorithm];
    } else {
        dec = &abs_decompressors[m_cfg->algorithm];
    }

    hdr = new (std::nothrow) char[sizeof(uint32_t) * (5 + 1 + 2)
          + dec->length]; 
    
    if (hdr == NULL) {
        std::cerr << ERR_PREAMBLE << "memory allocation failed" << std::endl;
        return -1;
    }

    ptr = write32be(hdr,HUNK_HEADER);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,1);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,0);
   
    // Reserve space for hunk size
    ptr = write32be(ptr,0xc0dedbad);        // Offset 20 needs patching

    // Reserve space for the First hunk is always HUNK_CODE..
    ptr = write32be(ptr,HUNK_CODE);
    ptr = write32be(ptr,0xdeadbeef);        // Offset 28 needs patching
    
    // Write the decompressor..
    for (n = 0; n < dec->length; n++) {
        *ptr++ = dec->code[n];
    }

    // write it to the file
    n = ptr - hdr;
    m_ofs.write(hdr,n);
    if (!m_ofs) {
        std::cerr << ERR_PREAMBLE << "write failed (" << std::strerror(errno) << ")" << std::endl;
        n = -1;
    }

    delete[] hdr;
    return n;

}

int target_amiga::save_header_overlay(const char* buf, int len)
{
    (void)buf;
    (void)len;

    std::cerr << ERR_PREAMBLE << "overlay decompressor not implemented yet\n";
    return -1;
}

int target_amiga::save_header_exe(const char* buf, int len)
{
    (void)buf;
    (void)len;

    const decompressor* dec;
    int n;
    int num_seg;
    char* hdr;
    char* ptr;
    
    if (m_cfg->max_match < 256) {
        dec = &exe_decompressors_255[m_cfg->algorithm];
    } else {
        dec = &exe_decompressors[m_cfg->algorithm];
    }

    num_seg = m_new_hunks.size();           // See src/hunk.cpp for content of the aux data 
    hdr = new (std::nothrow) char[sizeof(uint32_t) * (5 + num_seg + 10
        + (3 * num_seg)) + dec->length + 4]; 
    
    if (hdr == NULL) {
        std::cerr << ERR_PREAMBLE << "memory allocation failed" << std::endl;
        return -1;
    }

    ptr = write32be(hdr,HUNK_HEADER);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,num_seg);
    ptr = write32be(ptr,0);
    ptr = write32be(ptr,num_seg-1);

    for (n = 0; n < num_seg; n++) {
        ptr = write32be(ptr,m_new_hunks[n].mem_size_typed_longs);
    }
    
    // First hunk is always HUNK_CODE with 6 bytes of payload for a JSR
    ptr = write32be(ptr,m_new_hunks[0].hunk_type);
    ptr = write32be(ptr,2);    
    
    // JSR 0x00000000   b0100111010111001
    ptr = write16be(ptr,0x4eb9);        // JSR
    ptr = write32be(ptr,0x00000004);    // to 4th byte of the last segment
    ptr = write16be(ptr,0x4e70);        // RESET ;)

    // Followed by a single relocation to the last hunk, which will contain
    // our decompressor and the compressed data
    ptr = write32be(ptr,HUNK_RELOC32);
    ptr = write32be(ptr,1);
    ptr = write32be(ptr,num_seg-1);
    ptr = write32be(ptr,2);
    ptr = write32be(ptr,0);
    //ptr = write32be(ptr,HUNK_END);
    
    // Write rest of the hunks minus the last (with compressed data) with data size of 0..
    for (n = 1; n < num_seg-1; n++) {
        uint32_t hunk = m_new_hunks[n].hunk_type;

        ptr = write32be(ptr,hunk);
        
        if (hunk == HUNK_BSS) {
            ptr = write32be(ptr,m_new_hunks[n].mem_size_typed_longs & 0x3fffffff);
        } else {
            ptr = write32be(ptr,0x00000000);
        }
        //ptr = write32be(ptr,HUNK_END);
    }

    // the last HUNK_CODE to host the compressed file. We do not know its data size yet..
    ptr = write32be(ptr,HUNK_CODE);
    
    // record the location for patching the data size.. for now put 0 there.
    n = ptr - hdr;
    m_new_hunks[(num_seg - 1)].data_size_longs = n;
    ptr = write32be(ptr,0);

    // Write the decompressor..
    for (n = 0; n < dec->length; n++) {
        *ptr++ = dec->code[n];
    }

    // write it to the file
    n = ptr - hdr;
    m_ofs.write(hdr,n);
    if (!m_ofs) {
        std::cerr << ERR_PREAMBLE << "write failed (" << std::strerror(errno) << ")" << std::endl;
        n = -1;
    }

    delete[] hdr;
    return n;
}


/**
 * @brief The post compression header/trailer fixing function.
 *
 * This task of this function is to make the final adjustments to the compressed
 * executable. For each type of decompressor there are dedicated functions:
 * Normal executable decompressor, overlay executable decompressor and the
 * absolute address decompressor.
 *
 * @param[in] len The final length of the compressed file.
 * @return Final length of the output file or negative is an error took place.
 *
 * @note: These functions do seek around the output file and one has to make
 *        sure the seek positions are maintained when ever the decompression
 *        assembly routines are modified. There is no automated compile time
 *        glue for that. The process is manual.. sorry.
 */

int target_amiga::post_save_exe(int len)
{
    char tmp[4] = {0};
    int original_len = len;
    int n;

    len += exe_decompressors[m_cfg->algorithm].length;

    if (len % 4) {
        // must align to 4..
        n = 4 - (len % 4);
        m_ofs.write(tmp,n);
        len += n;
    }

    // Write HUNK_END
    write32be(tmp,HUNK_END,false);
    m_ofs.write(tmp,4);

    // Patch the HUNK_CODE size with the compressed data size in words..
    n = m_new_hunks.size();
    m_ofs.seekp(m_new_hunks[n-1].data_size_longs,std::ios_base::beg);
    write32be(tmp,len>>2,false);
    m_ofs.write(tmp,4);

    // Patch the decompressor with the original byte size..
    write32be(tmp,original_len,false);
    m_ofs.write(tmp,4);

    // Seek to the end end and return the final byte size
    m_ofs.seekp(0,std::ios_base::end);
    n = m_ofs.tellp();

    if (!m_ofs) {
        std::cerr << ERR_PREAMBLE << "post saving failed" << std::endl;
        n = -1;
    }

    return n;
}

int target_amiga::post_save_abs(int len)
{
    char tmp[4] = {0};
    int original_len = len;
    int n;
    
    len += abs_decompressors[m_cfg->algorithm].length;

    if (len % 4) {
        // must align to 4..
        n = 4 - (len % 4);
        m_ofs.write(tmp,n);
        len += n;
    }

    // Write HUNK_END
    write32be(tmp,HUNK_END,false);
    m_ofs.write(tmp,4);

    // Patch the HUNK_HEADER and the HUNK_CODE size with the compressed data size in words..
    write32be(tmp,len>>2,false);
    
    m_ofs.seekp(20,std::ios_base::beg);
    m_ofs.write(tmp,4);
    
    m_ofs.seekp(28,std::ios_base::beg);
    m_ofs.write(tmp,4);

    // Patch the decompressor with the original byte size..
    write32be(tmp,original_len,false);
    m_ofs.seekp(32+12,std::ios_base::beg);
    m_ofs.write(tmp,4);

    // Patch load address
    write32be(tmp,m_trg->load_addr,false);
    m_ofs.seekp(32+2,std::ios_base::beg);
    m_ofs.write(tmp,4);

    // Patch jump address
    write32be(tmp,m_trg->jump_addr,false);
    m_ofs.seekp(32+184,std::ios_base::beg);
    m_ofs.write(tmp,4);

    // Seek to the end end and return the final byte size
    m_ofs.seekp(0,std::ios_base::end);
    n = m_ofs.tellp();

    if (!m_ofs) {
        std::cerr << ERR_PREAMBLE << "post saving failed" << std::endl;
        n = -1;
    }

    return n;
}


int target_amiga::post_save_overlay(int len)
{
    (void)len;

    std::cerr << ERR_PREAMBLE << "post_save_overlay() not implemented\n";
    return -1;
}

int target_amiga::post_save(int len)
{
    if (m_nohunks) {
        return post_save_abs(len);
    }
    if (m_trg->overlay) {
        return post_save_overlay(len);
    }

    return post_save_exe(len);
}
