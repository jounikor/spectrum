/**
 * @file main.cpp
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 * @brief The main driver for the compressor.
 * @copyright The Unlicense
 *
 *
 *
 *
 *
 *
 */

#include <iosfwd>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <new>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <getopt.h>

#include "zxpac4.h"
#include "zxpac4_32k.h"
#include "zxpac4b.h"
#include "zxpac4c.h"
#include "lz_util.h"
#include "lz_base.h"
#include "hunk.h"
#include "target.h"

#include "version.h"

// Constants for LZ stuff
//
//


#define MAX_HEADER_OVERHEAD	ZXPAC4_HEADER_SIZE
#if ZXPAC4_32K_HEADER_SIZE > MAX_HEADER_OVERHEAD
  #define MAX_HEADER_OVERHEAD ZXPAC4_32K_HEADER_SIZE
#endif
#if ZXPAC4B_HEADER_SIZE > MAX_HEADER_OVERHEAD
  #define MAX_HEADER_OVERHEAD ZXPAC4B_HEADER_SIZE
#endif
#if ZXPAC4C_HEADER_SIZE > MAX_HEADER_OVERHEAD
  #define MAX_HEADER_OVERHEAD ZXPAC4C_HEADER_SIZE
#endif


#define DEF_OUTPUT_NAME     "zx.pac"
#define MAX_CHAIN	        9999
#define DEF_CHAIN           16
#define MAX_BACKWARD_STEPS  16
#define DEF_BACKWARD_STEPS  0

// Algorithms - these should be moved to somewhere common place
// accessible to different targets as well.
#define ZXPAC4              0
#define ZXPAC4B             1
#define ZXPAC4_32K          2
#define ZXPAC4C             3
#define ZXPAC_DEFAULT       ZXPAC4
#define ZXPAC_MAX           ZXPAC4C+1


static const char *algo_names[] = {
    "zxpac4",
    "zxpac4b",
    "zxpac4_32k",
    "zxpac4c",
};



static const char* def_filename = "SCOOPEX";


// Did not want to "reinvent" a command line parser thus
// we use the old trusty getopt..

static struct option longopts[] = {
    // string matcher parameterization
	{"max-chain",   required_argument,  NULL, 'c'},
	{"good-match",  required_argument,  NULL, 'g'},
	{"max-match",   required_argument,  NULL, 'm'},
	{"backward",    required_argument,  NULL, 'B'},
	{"only-better", no_argument,        NULL, 'b'},
    {"pmr-offset",  required_argument,  NULL, 'p'},
    // generic parameters
	{"reverse-file",no_argument,        NULL, 'R'},
	{"reverse-encoded",no_argument,     NULL, 'r'},
	{"verbose",     no_argument,        NULL, 'v'},
	{"DEBUG",       no_argument,        NULL, 'D'},
	{"debug",       no_argument,        NULL, 'd'},
	{"overlay",     no_argument,        NULL, 'O'},
    {"algo",        required_argument,  NULL, 'a'},
    {"abs",         required_argument,  NULL, 'A'},
    {"merge-hunks", no_argument,        NULL, 'M'},
    {"equalize-hunks", no_argument,     NULL, 'E'},
    {"list",        no_argument,        NULL, 'l'},
    {"help",        no_argument,        NULL, 'h'},
    {"file-name",   required_argument,  NULL, 'n'},
    {"preload",     required_argument,  NULL, 'L'},
    {0,0,0,0}
};

static void usage(char *prg, const targets::target* trg)
{
    (void)trg;

    std::cerr << "ZXPAC4 v" << ZXPAC4_MAJOR << "." << ZXPAC4_MINOR << " (c) 2022-24 Jouni 'Mr.Spiv' Korhonen\n\n";
    std::cerr << "Usage: " << prg << " target [options] infile [outfile]\n";
	std::cerr << " Targets:\n";
    std::cerr << "  bin - Binary data file\n"
              << "  asc - 7bit ASCII data file\n"
              << "  zx  - ZX Spectrum self executing TAP\n"
              << "  bbc - BBC Micro self executing program\n"
              << "  ami - Amiga self executing program\n";
    std::cerr << " Options:\n";
    std::cerr << "  --max-chain,-c num    Maximum number of stored matches per position "
              << "(min 1, max " << MAX_CHAIN << ").\n";
    std::cerr << "  --good-match,-g num   Match length that cuts further searches.\n";
    std::cerr << "  --max-match,-m len    Set the maximum match length. Default is an algorithm specific.\n"
              << "                        (Note, this is an expert option. You better know what you are doing).\n";
    std::cerr << "  --backward,-B num     Number of backward steps after a found match "
              << "(min 0, max " << MAX_BACKWARD_STEPS << ").\n";
    std::cerr << "  --only-better,-b      Further matches in the history must always be better than previous\n"
              << "                        matches for the same position (default no).\n";
    std::cerr << "  --pmr-offset,-p       Initial PMR offset between 1 and 63 (default depends on the target).\n";
    std::cerr << "  --reverse-encoded,-r  Reverse the encoded file to allow end-to-start decompression\n"
              << "                        (default no reverse)\n";
    std::cerr << "  --reverse-file,-R     Reverse the input file to allow end-to-start decompression. This \n"
              << "                        setting will enable '--reverse-encoded' as well (default no reverse)\n";
    std::cerr << "  --algo,-a             Select used algorithm (default depends on the target):\n"
              << "                          0=zxpac4      LZSS, 128K window\n"
              << "                          1=xzpac4b     LZSS, literal runs, 128K window\n"
              << "                          2=zxpac4_32k  Same as zxpac4 with 32K window\n"
              << "                          3=zxpac4c     LZSS, literal runs, 128K window, tANS backend, no ascii\n";
    std::cerr << "  --preshift,-P         Preshift the last ASCII literal (requires 'asc' target):\n";
    std::cerr << "  --abs,-A load,jump    Self-extracting decruncher parameters for absolute address location.\n";
    std::cerr << "  --merge-hunks,-M      Merge hunks (Amiga target).\n";
    std::cerr << "  --equalize-hunks,-E   Treat HUNK_CODE/DATA/BSS all the same. This setting will enable\n"
              << "                        '--merge-hunks' as well (Amiga target).\n";
    std::cerr << "  --overlay,-O          Self-extracting overlay decruncher (Amiga target).\n";
    std::cerr << "  --debug,-d            Output a LOT OF debug prints to stderr.\n";
    std::cerr << "  --DEBUG,-D            Output EVEN MORE debug prints to stderr.\n";
    std::cerr << "  --verbose,-v          Output some additional information to stdout.\n";
    std::cerr << "  --file-name,-n        Filename, for example, for ZX Spectrum TAP file.\n";
    std::cerr << "  --preload,-L file     Preload tANS scaled frequencies for ZXPac4c\n";
	std::cerr << "  --preset profile      Preset internal tNAS profile for ZXPac4c:\n"
			  << "                          0=default\n"
			  << "                          1=Amiga exe)\n";
    std::cerr << "  --list,-l             Print defaults & details of each supported target and algorithm.\n";
    std::cerr << "  --help,-h             Print this output ;)\n";
    std::cerr << std::flush;
    exit(EXIT_FAILURE); 
}


static targets::target my_targets[] = {
    {   "asc",
        "Draft: 7-bit ASCII only target. 8-bit input causes an error.",
        (1<<24) - 1,
        1<<ZXPAC4 | 1<<ZXPAC4B | 1<<ZXPAC4_32K | 1<<ZXPAC4C,
        ZXPAC4,
        0,
        0x0,
        0x0,
        NULL,
        4,          // initial_pmr
        TRG_NSUP,      // overlay
        TRG_NSUP,      // merge_hunks
        TRG_NSUP,      // equalize_hunks
        TRG_NSUP,      // encode_to_ram
    },
    {   "bin",
        "Draft: 8-bit binary data target.",
        (1<<24) - 1,
        1<<ZXPAC4 | 1<<ZXPAC4B | 1<<ZXPAC4_32K | 1<<ZXPAC4C,
        ZXPAC4,
        0,
        0x0,
        0x0,
        NULL,
        4,          // initial_pmr
        TRG_NSUP,      // overlay
        TRG_NSUP,      // merge_hunks
        TRG_NSUP,      // equalize_hunks
        TRG_NSUP,      // encode_to_ram
    },
    {   "zx",
        "Draft: A TAP file contains a decompressor and runs the compressed program.",
        (1<<16) - 1,
        1<<ZXPAC4_32K,
        ZXPAC4_32K | 1<<ZXPAC4C,
        255,
        0x0,
        0x0,
        def_filename,
        4,              // initial_pmr
        TRG_NSUP,       // overlay
        TRG_NSUP,       // merge_hunks
        TRG_NSUP,       // equalize_hunks
        TRG_NSUP,       // encode_to_ram
    },
    {   "bbc",
        "Draft: BBC Model A/B self-extracting executable file.",
        (1<<16) - 1,
        1<<ZXPAC4 | 1<<ZXPAC4B | 1<<ZXPAC4_32K,
        ZXPAC4_32K,
        255,
        0x0,
        0x0,
        def_filename,
        4,              // initial_pmr
        TRG_NSUP,       // overlay
        TRG_NSUP,       // merge_hunks
        TRG_NSUP,       // equalize_hunks
        TRG_NSUP,       // encode_to_ram
    },
    {   "ami",
        "Amiga compressed executable.",
        (1<<24) - 1,
        1<<ZXPAC4 | 1<<ZXPAC4_32K | 1<<ZXPAC4C,
        ZXPAC4,
        0,
        0x0,        // load address
        0x0,        // jump address
        NULL,
        4,          // initial_pmr
        TRG_NSUP,       // overlay
        TRG_FALSE,      // merge_hunks
        TRG_FALSE,      // equalize_hunks
        TRG_FALSE,      // encode_to_ram
    }   
};


static const lz_config algos[] {
    // ZXPAC4
    {   ZXPAC4_WINDOW_MAX,  128, DEF_CHAIN, ZXPAC4_MATCH_MIN, ZXPAC4_MATCH_MAX, ZXPAC4_MATCH_GOOD,
        1,
		DEF_BACKWARD_STEPS, ZXPAC4_OFFSET_MATCH2_THRESHOLD, ZXPAC4_OFFSET_MATCH3_THRESHOLD,
        ZXPAC4_INIT_PMR_OFFSET,
        DEBUG_LEVEL_NONE,
        ZXPAC4,
        false,      // only_better_matches
        LZ_CFG_FALSE,      // reverse_file
        LZ_CFG_FALSE,      // reverse_encoded
        LZ_CFG_FALSE,      // is_ascii
        LZ_CFG_FALSE,      // preshift_last_ascii_literal
        false       // verbose
    },
    // ZXPAC4B
    {   ZXPAC4B_WINDOW_MAX,  128, DEF_CHAIN, ZXPAC4B_MATCH_MIN, ZXPAC4B_MATCH_MAX, ZXPAC4B_MATCH_GOOD,
        255,
		DEF_BACKWARD_STEPS, ZXPAC4B_OFFSET_MATCH2_THRESHOLD, ZXPAC4B_OFFSET_MATCH3_THRESHOLD,
        ZXPAC4B_INIT_PMR_OFFSET,
        DEBUG_LEVEL_NONE,
        ZXPAC4B,
        false,      // only_better_matches
        LZ_CFG_FALSE,      // reverse_file
        LZ_CFG_FALSE,      // reverse_encoded
        LZ_CFG_FALSE,      // is_ascii
        LZ_CFG_FALSE,      // preshift_last_ascii_literal
        false       // verbose
    },
    // ZXPAC4_32K - max 32K window
    {   ZXPAC4_32K_WINDOW_MAX,  128, DEF_CHAIN, ZXPAC4_32K_MATCH_MIN, ZXPAC4_32K_MATCH_MAX, ZXPAC4_32K_MATCH_GOOD,
        1,
		DEF_BACKWARD_STEPS, ZXPAC4_32K_OFFSET_MATCH2_THRESHOLD, ZXPAC4_32K_OFFSET_MATCH3_THRESHOLD,
        ZXPAC4_32K_INIT_PMR_OFFSET,
        DEBUG_LEVEL_NONE,
        ZXPAC4_32K,
        false,      // only_better_matches
        LZ_CFG_FALSE,      // reverse_file
        LZ_CFG_FALSE,      // reverse_encoded
        LZ_CFG_FALSE,      // is_ascii
        LZ_CFG_FALSE,      // preshift_last_ascii_literal
        false       // verbose
    },
    // ZXPAC4C - max 128K window, literal runs, 
    {   ZXPAC4C_WINDOW_MAX,  256, DEF_CHAIN, ZXPAC4C_MATCH_MIN, ZXPAC4C_MATCH_MAX, ZXPAC4C_MATCH_GOOD,
        255,
		DEF_BACKWARD_STEPS, ZXPAC4C_OFFSET_MATCH2_THRESHOLD, ZXPAC4C_OFFSET_MATCH3_THRESHOLD,
        ZXPAC4C_INIT_PMR_OFFSET,
        DEBUG_LEVEL_NONE,
        ZXPAC4C,
        false,          // only_better_matches
        LZ_CFG_TRUE|LZ_CFG_CONST,           // reverse_file
        LZ_CFG_TRUE|LZ_CFG_CONST,           // reverse_encoded
        LZ_CFG_NSUP,    // is_ascii
        LZ_CFG_FALSE|LZ_CFG_CONST,          // preshift_last_ascii_literal
        false           // verbose
    }
};

#define LZ_TARGET_SIZE static_cast<int>((sizeof(my_targets)/sizeof(targets::target)))
#define LZ_ALGO_SIZE static_cast<int>((sizeof(algos)/sizeof(lz_config)))

/**
 * @brief List all supported targets and their details.
 * @param[in]   a ptr to the shell command name.
 *
 * @return  VOID
 *
 */
void list(const char* argv, const targets::target* trg)
{
    (void)argv;
    int mm;

    std::cout << "Details for target '" << trg->target_name << "':\n";
    std::cout << "  Brief: " << trg->target_brief << "\n";
    std::cout << "  Supported allgorithms:\n";
    for (int i = 0; i < ZXPAC_MAX; i++) {
        if (trg->supported_algorithms & (1 << i)) {
            std::cout << "    " << algo_names[i] << "\n";
        }
    }
    std::cout << "  Default algorithm: " << algo_names[trg->algorithm] << "\n";
    
    mm = trg->max_match;;
    if (mm == 0) {
        std::cout << "  Maximum match length uses selected algorithm default\n";
    } else {
        std::cout << "  Maximum match length: " << mm << "\n";
    }
    
    mm = trg->initial_pmr;
    if (mm == 0) {
        std::cout << "  Initial PMR offset uses selected algorithm default\n";
    } else {
        std::cout << "  Initial PMR offset: " << mm << "\n";
    }

    if (trg->overlay != TRG_NSUP) {
        std::cout << "  Amiga specific executable file decompression using overlay supported\n";
    }
    if (trg->merge_hunks != TRG_NSUP) {
        std::cout << "  Executable file hunk/segment merging supported\n";
    }
    if (trg->equalize_hunks != TRG_NSUP) {
        std::cout << "  Amiga specific executable file HUNK_CODE/DATA/BSS merging supported\n";
    }

    for (int i = 0; i < ZXPAC_MAX; i++) {
        if (trg->supported_algorithms & (1 << i)) {
            std::cout << algo_names[i] << " details and defaults:\n";
        
            std::cout << "  Window size: " << algos[i].window_size << "\n";
            std::cout << "  Default length of hash linked list of matches: " << algos[i].max_chain << "\n";
            std::cout << "  Minimum match length: " << algos[i].min_match << "\n";
            std::cout << "  Maximum match length: " << algos[i].max_match << "\n";
            std::cout << "  Default good match length: " << algos[i].good_match << "\n";
            std::cout << "  Default PMR offset: " << algos[i].initial_pmr_offset << "\n";
            if (algos[i].reverse_encoded & LZ_CFG_TRUE) {
                std::cout << "  Backwards decompression ";
				if (algos[i].reverse_encoded & LZ_CFG_CONST) {
					std::cout << "forced";
				} else {
					std::cout << "set but changeable";
				}
				std::cout << "\n";
            }
            if (algos[i].reverse_file & (LZ_CFG_TRUE)) {
                std::cout << "  File reversed for backwards decompression ";
				if (algos[i].reverse_encoded & LZ_CFG_CONST) {
					std::cout << "forced";
				} else {
					std::cout << "set but changeable";
				}
				std::cout << "\n";
            }
            if (algos[i].is_ascii != LZ_CFG_NSUP) {
                std::cout << "  7-bit ASCII compression mode supported\n";
}   }   }   }



/**
 *
 *
 *
 *
 *
 */
static void reverse_buffer(char* p_buf, int len) {
    char t;
    for (int n = 0; n < len/2; n++) {
        t = p_buf[n];
        p_buf[n] = p_buf[len-n-1];
        p_buf[len-n-1] = t;
    }
}


/**
 * @brief A factory function to instantiate a required target.
 * @param[in]    trg_name A ptr to the target name C-string.
 * @param[inout] cfg A ptr to LZ-config.
 * @param[in]    ofd A reference to output file.
 *
 * @return A ptr to target object.
 */
static target_base* create_target(const targets::target* trg, lz_config* cfg, std::ofstream& ofs)
{
    target_base* ptr;

    if (!(strcmp(trg->target_name,"asc"))) {
        ptr = new (std::nothrow) target_ascii(trg,cfg,ofs);
    } else if (!(strcmp(trg->target_name,"bin"))) {
        ptr = new (std::nothrow) target_binary(trg,cfg,ofs);
    } else if (!(strcmp(trg->target_name,"ami"))) {
        ptr = new (std::nothrow) target_amiga(trg,cfg,ofs);
    } else if (!(strcmp(trg->target_name,"zx"))) {
        ptr = new (std::nothrow) target_spectrum(trg,cfg,ofs);
    } else if (!(strcmp(trg->target_name,"bbc"))) {
        ptr = new (std::nothrow) target_bbc(trg,cfg,ofs);
    } else {
        ptr = NULL;
    }
    return ptr;
}

/**
 * @brief Driver function for a generic LZ compression..
 * @param[in] trg A const ptr to targets::target for this file.
 * @param[in] lz  A ptr to lz_base for this file.
 * @param[in] cfg A ptr to lz_cinfig for this file.
 * @param[in] ifs A reference to input file (=this file).
 * @param[in] ofs A reference to output file.
 * @param[in] len The length of the input file (=length of this file).
 *                (This is actually redundant information).
 *
 * @return Final saved file length or negative in case of an error.
 */
static int handle_file(const targets::target* trg, lz_base* lz, lz_config_t* cfg, std::ifstream& ifs, std::ofstream& ofs, int len)
{
    int n = 0;
    // extra N characters to avoid buffer overrun with 3 byte hash function..
    char* buf = new (std::nothrow) char[len+3];
    char* p_out = NULL;
    target_base* trg_ptr = create_target(trg,cfg,ofs);

    if (buf == NULL) {
        std::cerr << ERR_PREAMBLE << "failed to allocate memory for the input file\n";
        n = -1;
		goto error_exit;
    }
    if (trg_ptr == NULL) {
        std::cerr << ERR_PREAMBLE << "failed to instantiate a target object\n";
        n = -1;
		goto error_exit;
    }
	if (( p_out = new(std::nothrow) char[len+MAX_HEADER_OVERHEAD]) == NULL) {
		std::cerr << "**Error: Allocating memory for the file failed" << std::endl;
		n = -1;
		goto error_exit;
	}
    if (!ifs.read(buf,len)) {
        std::cerr << ERR_PREAMBLE << "reading the input file failed\n";
        n = -1;
		goto error_exit;
    }
    
    len = trg_ptr->preprocess(buf,len);
    if (cfg->verbose) {
        std::cout << "File size after preprocessing is " << len << std::endl;
    }
    if (len < 0) {
        std::cerr << ERR_PREAMBLE << "Preprocessing failed" << std::endl;
        n = len;
        goto error_exit;
    }
    if ((n = trg_ptr->save_header(buf,len)) < 0) {
        std::cerr << ERR_PREAMBLE << "Saving file header failed" << std::endl;
        goto error_exit;
    }
	if (cfg->reverse_file) {
		if (cfg->verbose) {
			std::cout << "Reversing the file for backwards decompression" << std::endl;
		}
		reverse_buffer(buf,len);
	}

    lz->lz_search_matches(buf,len,0); 
    lz->lz_parse(buf,len,0); 

    if (cfg->verbose) {
        std::cout << "Encoding the compressed file" << std::endl;
    }

	n = lz->lz_encode(buf,len,p_out,NULL);
    
	if (n > 0) {
        if (cfg->reverse_encoded) {
            if (cfg->verbose) {
                std::cout << "Reversing the encoded file.." << std::endl;
            }
            reverse_buffer(p_out,n);
        } 
        if (cfg->verbose) {
            std::cout << "Compressed length: " << n << std::endl;
        }
    } else {
        if (cfg->verbose) {
            std::cout << "Compression failed.." << std::endl;
			n = -1;
			goto error_exit;
        }
    }
    if (!(ofs.write(p_out,n))) {
        std::cerr << ERR_PREAMBLE << "writing compressed file failed" << std::endl;
        n = -1;
		goto error_exit;
    }
    if ((n = trg_ptr->post_save(p_out,n)) < 0) {
        std::cerr << ERR_PREAMBLE << "Post save failed" << std::endl;
    }
error_exit: 
    delete trg_ptr;
    delete[] buf;
	delete[] p_out;
    return n;
}


int main(int argc, char** argv)
{
    int n;
    int file_len = -1;
    char* endptr;
    int compressed_len = 0;
    
    std::ifstream ifs;
    std::ofstream ofs;

    int cfg_debug_level = DEBUG_LEVEL_NONE;
    bool cfg_preshift = false; 
    bool cfg_verbose_on = false;
    std::string cfg_infile_name;
    std::string cfg_outfile_name(DEF_OUTPUT_NAME);
    int cfg_algo = ZXPAC_DEFAULT;
    int cfg_good_match = -1;
    int cfg_backward_steps = -1;
    int cfg_initial_pmr_offset = -1;
    int cfg_max_chain = -1;
    int cfg_max_match = -1;
    bool cfg_only_better_matches = false;
    bool cfg_reverse_file = false;
    bool cfg_reverse_encoded = false;
    bool trg_merge_hunks = false;
    bool trg_equalize_hunks = false;
    bool trg_overlay = false;
    uint32_t trg_load_addr = 0;
    uint32_t trg_jump_addr = 0;
    lz_base* lz = NULL;
    const char* trg_file_name = NULL;

    targets::target* trg = NULL;

    // Check target..

    if (argc < 2) {
        usage(argv[0],&my_targets[0]);
    }

    for (n = 0; n < LZ_TARGET_SIZE; n++) {
        if (!strcmp(argv[1],my_targets[n].target_name)) {
            trg = &my_targets[n];
            break;
        }
    }

    if (trg == NULL) {
        usage(argv[0],&my_targets[0]);
    }

    // target specific default algo
    cfg_algo = trg->algorithm;
    cfg_initial_pmr_offset = trg->initial_pmr;
    optind = 2;

    // 
	while ((n = getopt_long(argc, argv, "Em:g:c:e:B:i:s:p:hPvdDa:A:OMrRb:n:lL:", longopts, NULL)) != -1) {
		switch (n) {
            case 'O':   // --overlay
                trg_overlay = true;
                break;
            case 'h':
                usage(argv[0],trg);
                break;
			case 'P':   // --preshift
                cfg_preshift = true;
				break;
			case 'd':   // --debug
                cfg_debug_level = DEBUG_LEVEL_NORMAL;
				break;
			case 'D':   // --DEBUG
                cfg_debug_level = DEBUG_LEVEL_EXTRA;
				break;
			case 'v':   // --verbose
                cfg_verbose_on = true;
				break;
            case 'a':   // --algo
                cfg_algo = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg_algo < 0 || cfg_algo > LZ_ALGO_SIZE) {
                    std::cerr << ERR_PREAMBLE << "Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0],trg);
                }
                if (!(trg->supported_algorithms & (1 << cfg_algo))) {
                    std::cerr << ERR_PREAMBLE << "not supported algorithm "
                        << "for the target" << std::endl;
                    usage(argv[0],trg);
                }
                break;
            case 'r':   // --reverse-encoded
                cfg_reverse_encoded = true;
                break;
            case 'R':   // --reverse-file
                cfg_reverse_encoded = true;
                cfg_reverse_file = true;
                break;
            case 'b':   // --only-better
                cfg_only_better_matches = true;
                break;
            case 'p':   // --pmr-offset
                cfg_initial_pmr_offset = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg_initial_pmr_offset > 63 || cfg_initial_pmr_offset < 1) {
                    std::cerr << ERR_PREAMBLE << "Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0],trg);
                }
                break;
            case 'c':   // --max-chain
                cfg_max_chain = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg_max_chain > MAX_CHAIN || cfg_max_chain < 1) {
                    std::cerr << ERR_PREAMBLE << "Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0],trg);
                }
                break;
            case 'm':   // --large-max-match
                cfg_max_match = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg_max_match < 2) {
                    std::cerr << ERR_PREAMBLE << "Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0],trg);
                }
                break;
            case 'g':   // --good-match
                cfg_good_match = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0') {
                    std::cerr << ERR_PREAMBLE << "Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0],trg);
                }
                break;
            case 'B':   // --backsteps
                cfg_backward_steps = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg_backward_steps > MAX_BACKWARD_STEPS || cfg_backward_steps < 0) {
                    std::cerr << ERR_PREAMBLE << "Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0],trg);
                }
                break;
            case 'A':
                n = sscanf(optarg,"%x,%x",&trg_load_addr,&trg_jump_addr);

                if (n != 2) {
                    usage(argv[0],trg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'M':   // --merge-hunks
                trg_merge_hunks = true;
                break;
            case 'E':   // --equalize-hunks
                trg_equalize_hunks = true;
                trg_merge_hunks = true;
                break;
            case 'n':   // --file-name
                trg_file_name = optarg;
                break;
            case 'l':   // --list
                list(argv[0],trg);
                exit(EXIT_FAILURE);
            case 'L':
                std::cerr << "--preload is not suppoprted yet\n";
                exit(EXIT_FAILURE);
            case '?':
			case ':':
				usage(argv[0],trg);
				exit(EXIT_FAILURE);
			default:
				break;
		}
	}

	if (argc - optind < 1) {
		usage(argv[0],trg);
		exit(EXIT_FAILURE);
	}

    // handle lz_config
    lz_config cfg = algos[cfg_algo];
    
    if (cfg_good_match > -1) {
        if (cfg_good_match > cfg.max_match || 
            cfg_good_match < cfg.min_match) {
            std::cerr << ERR_PREAMBLE << "Invalid parameter value '" << cfg_good_match  << "'\n";
            usage(argv[0],trg);
        }
        cfg.good_match = cfg_good_match;
    }
    if (cfg_backward_steps > -1) {
        cfg.backward_steps = cfg_backward_steps;
    }
    if (cfg_initial_pmr_offset > 0) {
        cfg.initial_pmr_offset = cfg_initial_pmr_offset;
    }
    if (cfg_max_chain > -1) {
        cfg.max_chain = cfg_max_chain;
    }
    if (trg_overlay && (trg_load_addr || trg_jump_addr)) {
        trg_overlay = false;
        if (cfg_verbose_on) {
            std::cout << "**Warning: absolute address decompression overrides overlay\n";
        }
    }
    if (trg->max_match > 0) {
        if (cfg_max_match > trg->max_match || cfg_max_match < 0) {
            cfg_max_match = trg->max_match;
            if (cfg_verbose_on) {
                std::cout << "**Warning: maximum match too big for the target. "
                          << "Truncating to " << cfg_max_match << ".\n";
            }
        }
    }
    if (cfg_max_match > cfg.max_match) {
        cfg_max_match = cfg.max_match;
        if (cfg_verbose_on) {
            std::cout << "**Warning: maximum match too big. "
                      << "Truncating to " << cfg_max_match << ".\n";
        }
    }
    if (cfg_max_match > 0) {
        cfg.max_match = cfg_max_match;
    }
    
    // We do not check for all possible dump combinations..
    cfg.only_better_matches = cfg_only_better_matches;
    
	if ((cfg.preshift_last_ascii_literal & LZ_CFG_CONST) && cfg_preshift) {
		std::cout << "**Warning: -P,--preshift not applicable for this target\n";
	} else {
		cfg.preshift_last_ascii_literal = LZ_CFG_TRUE;
	}
	if ((cfg.reverse_file & LZ_CFG_CONST) && cfg_reverse_file) {
		std::cout << "**Warning: -R,--reverse-file not applicable for this target\n";
	} else {
		cfg.reverse_file = LZ_CFG_TRUE;
    }
	if ((cfg.reverse_encoded & LZ_CFG_CONST) && cfg_reverse_encoded) {
		std::cout << "**Warning: -r,--reverse-encoded not applicable for this target\n";
	} else {
		cfg.reverse_encoded = LZ_CFG_TRUE;
	}

    trg->merge_hunks = trg_merge_hunks;
    trg->equalize_hunks = trg_equalize_hunks;
    trg->overlay = trg_overlay;
    trg->load_addr = trg_load_addr;
    trg->jump_addr = trg_jump_addr;
    cfg.verbose = cfg_verbose_on;
    cfg.debug_level = cfg_debug_level;
    
    // file name
    if (trg_file_name == NULL) {
        trg->file_name = argv[optind];
    } else {
        trg->file_name = trg_file_name;
    }

    // The following stuff is horrible.. Needs to be hidden under
    // multiple functions to avoid recurring cleanup due initialization
    // failures..

    cfg_infile_name  = std::string(argv[optind++]);

    if (argc - optind >= 1) {
        cfg_outfile_name = std::string(argv[optind++]);
    }

    ifs.open(cfg_infile_name,std::ios::binary|std::ios::in|std::ios::ate);
    if (ifs.is_open() == false) {
        std::cerr << ERR_PREAMBLE << "failed to open input file '" << cfg_infile_name << "'\n";
        goto error_exit;
    }

    file_len = ifs.tellg();
    ifs.seekg(0);

    if (file_len < 0) {
        std::cerr << ERR_PREAMBLE << "getting file length  of '" << cfg_infile_name 
            << "' failed" << std::endl;
        goto error_exit;
    }
    if (file_len > trg->max_file_size) {
        std::cerr << ERR_PREAMBLE << "maximum file length is " << trg->max_file_size
            << " bytes" << std::endl;
        goto error_exit;
    }

    try {
        switch (cfg_algo) {
        case ZXPAC4B:
            lz = new zxpac4b(&cfg);
            break;
        case ZXPAC4_32K:
            lz = new zxpac4_32k(&cfg);
            break;
        case ZXPAC4C:
            lz = new zxpac4c(&cfg); 
            break;
        case ZXPAC4:
        default:
            lz = new zxpac4(&cfg);
            break;
        }
    } catch (std::exception& e) {
        std::cerr << ERR_PREAMBLE << e.what() << "\n";
        lz = NULL;
        goto error_exit;
    }
    try {
        lz->lz_cost_array_get(file_len);
    } catch (std::exception& e) {
        std::cerr << ERR_PREAMBLE << e.what() << "\n";
        goto error_exit;
    }
   
    cfg.algorithm = cfg_algo;
    lz->set_debug_level(cfg_debug_level);
    lz->enable_verbose(cfg_verbose_on);
    
    if (cfg_verbose_on) {
        std::cout << "Loading from file '" << cfg_infile_name << "'\n";
        std::cout << "File length is " << file_len << "\n";
        std::cout << "Saving to file '" << cfg_outfile_name << "'\n";
        std::cout << "Using target '" << trg->target_name << "' and algorithm " << cfg.algorithm << "\n";
        std::cout << "Min match is " << cfg.min_match << "\n";
        std::cout << "Max match is " << cfg.max_match << "\n";
        std::cout << "Good match is " << cfg.good_match << "\n";
    }

    ofs.open(cfg_outfile_name,std::ios::binary|std::ios::out);
    if (ofs.is_open()) {
        compressed_len = handle_file(trg,lz,&cfg,ifs,ofs,file_len);
        
        if (compressed_len < 0) {
            std::cerr << ERR_PREAMBLE << "compression failed\n";
            goto error_exit;
        }

        double gain = 1.0 - static_cast<double>(compressed_len) / static_cast<double>(file_len);
        std::cout << std::dec << "Original: " << file_len << ", compressed: " << std::setprecision(4)
                  << compressed_len << ", gained: " << gain*100 << "%\n";

        if (cfg_verbose_on) {
            std::cout << "Number of literals: " << lz->get_num_literals() << std::endl;
            std::cout << "Number of matches: " << lz->get_num_matches() << std::endl;
            std::cout << "Number of matched bytes: " << lz->get_num_matched_bytes() << std::endl;
            std::cout << "Number of PMR matches: " << lz->get_num_pmr_matches() << std::endl; 
            std::cout << "Number of PMR literals: " << lz->get_num_pmr_literals() << std::endl;
            std::cout << "Security distance: " << lz->get_security_distance() << std::endl;
        }
    } else {
        std::cerr << ERR_PREAMBLE << "opening output file '" << cfg_outfile_name << "' failed\n";
    }

error_exit:
    if (lz) {
        lz->lz_cost_array_done();
        delete lz;
    }

    ifs.close();
    ofs.close();
    return 0;
}
