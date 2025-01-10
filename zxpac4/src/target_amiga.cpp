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


#define AMIGA_EXE_SECURITY_DISTANCE     64

using namespace amiga_hunks;

// First 4 bytes is always reserved for the final compressed size of the file
#include "../m68k/zxpac4_exe.h"
#include "../m68k/zxpac4_32k_exe.h"

// Length must be even
static struct decompressor {
    int length;
    uint8_t* code;
} decompressors[] = {
    {sizeof(zxpac4_exe_bin),zxpac4_exe_bin,
    },
    {sizeof(zxpac4_32k_exe_bin),zxpac4_32k_exe_bin,
    },
    {32,NULL,   // This is still missing..
    }
};



target_amiga::target_amiga(const targets::target* trg, const lz_config_t* cfg, std::ofstream& ofs) : target_base(trg,cfg,ofs) {
    // check target and config.. some parameter changes based settings
    // force reverse file if not an overlaid decompression used
    
    if (!trg->overlay) {
        if (cfg->verbose) {
            std::cout << "Forcing reverse file and encoding for Amiga target\n";
        }

        cfg->reverse_file = true;
        cfg->reverse_encoded = true;
    }
    if (trg->load_addr == 0 && trg->jump_addr == 0) {
        m_nohunks = false;
    } else {
        m_nohunks = true;
    }
}

target_amiga::~target_amiga(void) {
}

int target_amiga::preprocess(char* buf, int len)
{
    std::vector<amiga_hunks::hunk_info_t> hunk_list(0);
    bool debug_on = m_cfg->debug_level > DEBUG_LEVEL_NONE ? true : false;
    char* amiga_exe = NULL;
    int m, n, memory_len;

    if (m_nohunks) {
        return len;
    }
    
    n = amiga_hunks::parse_hunks(buf,len,hunk_list,debug_on);
    
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
            memory_len = n + decompressors[m_cfg->algorithm].length + AMIGA_EXE_SECURITY_DISTANCE;

            // Fabricate a new hunk and insert the size of the compressed data
            m_new_hunks.push_back((memory_len+3) >> 2);
            m_new_hunks.push_back(0xdeadbeef);
            m_new_hunks.push_back(0xc0dedbad);
        }
        
        delete[] amiga_exe;
    }
    return n;
}

int target_amiga::save_header(const char* buf, int len)
{
    (void)buf;
    (void)len;

    int n;
    int num_seg;
    char* hdr;
    char* ptr;
    
    if (m_nohunks) {
        std::cerr << ERR_PREAMBLE << "absolute addresses not supported yet\n";
        return -1;
    }

    num_seg = m_new_hunks.size() / 3;     // See src/hunk.cpp for content of the aux data 
    assert(m_new_hunks.size() % 3 == 0);
    hdr = new (std::nothrow) char[sizeof(uint32_t) * (5 + num_seg + 10
        + (3 * num_seg)) + decompressors[m_cfg->algorithm].length + 4]; 
    
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
        ptr = write32be(ptr,m_new_hunks[3*n]);    
    }
    
    // First hunk is always HUNK_CODE with 6 bytes of payload for a JSR
    ptr = write32be(ptr,m_new_hunks[3*0 + 1]);
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
    ptr = write32be(ptr,HUNK_END);
    
    // Write rest of the hunks minus the last (with compressed data) with data size of 0..
    for (n = 1; n < num_seg-1; n++) {
        uint32_t hunk = m_new_hunks[3*n + 1];

        ptr = write32be(ptr,hunk);
        
        if (hunk == HUNK_BSS) {
            ptr = write32be(ptr,m_new_hunks[3*n]);
        } else {
            ptr = write32be(ptr,0x00000000);
        }
        ptr = write32be(ptr,HUNK_END);
    }

    // the last HUNK_CODE to host the compressed file. We do not know its data size yet..
    ptr = write32be(ptr,HUNK_CODE);
    
    // record the location for patching the data size.. for now put 0 there.
    n = ptr - hdr;
    m_new_hunks[num_seg*3 - 1] = n;
    ptr = write32be(ptr,0);

    // Write the decompressor..
    for (n = 0; n < decompressors[m_cfg->algorithm].length; n++) {
        *ptr++ = decompressors[m_cfg->algorithm].code[n];
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
 * @param[in] cfg A ptr to generic configuration.
 * @param[in] len The final length of the compressed file.
 * @param     m_ofs A reference to output file stream.
 * @param[in] A ptr to target specific auxilatory data.
 *
 * @return Final length of the output file or negative is an error took place.
 */

int target_amiga::post_save(int len)
{
    char tmp[4] = {0};
    int original_len = len;
    int n;

    len += decompressors[m_cfg->algorithm].length;

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
    m_ofs.seekp(m_new_hunks[n-1],std::ios_base::beg);
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

