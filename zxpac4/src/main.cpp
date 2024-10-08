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
#include "zxpac4b.h"
#include "lz_util.h"
#include "lz_base.h"

// Constants for LZ stuff
//
//

#define DEF_ALGO            "zxpac4"
#define DEF_OUTPUT_NAME     "zx.pac"
#define MAX_CHAIN	        9999
#define DEF_CHAIN           16
#define MAX_BACKWARD_STEPS  16
#define DEF_BACKWARD_STEPS  0

#define ZXPAC4              0
#define ZXPAC4B             1


// Did not want to "reinvent" a command line parser thus
// we use the old trusty getopt..

static struct option longopts[] = {
    // string matcher parameterization
	{"max-chain",   required_argument,  NULL, 'c'},
	{"good-match",  required_argument,  NULL, 'g'},
	{"backward",    required_argument,  NULL, 'B'},
	{"only-better", no_argument,        NULL, 'b'},
    {"pmr-offset",  required_argument,  NULL, 'p'},
    // generic parameters
	{"reverse",     no_argument,        NULL, 'r'},
	{"verbose",     no_argument,        NULL, 'v'},
	{"DEBUG",       no_argument,        NULL, 'D'},
	{"debug",       no_argument,        NULL, 'd'},
    {"algo",        required_argument,  NULL, 'a'},
    {"abs",         required_argument,  NULL, 'A'},
    {"exe",         no_argument,        NULL, 'X'},
    {"help",        no_argument,        NULL, 'h'},
    {0,0,0,0}
};

void usage( char *prg ) {
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
    std::cerr << "  --good-match,-g num   match length that cuts further searches.\n";
    std::cerr << "  --backward,-B num     Number of backward steps after a found match "
              << "(min 0, max " << MAX_BACKWARD_STEPS << ").\n";
    std::cerr << "  --only-better,-b      Further matches in the history must always be better than "
              << "previous matches for\n"
              << "                        the same position (default no).\n";
    std::cerr << "  --pmr-offset,-p       Initial PMR offset between 1 and 127 (default depends on the target).\n";
    std::cerr << "  --reverse,-r          Reverse the input file to allow end-to-start decompression "
              << "(default no reverse)\n";
    std::cerr << "  --algo,-a             Select used algorithm (0=zxpac4, 1=xzpac4b) (default is " << ZXPAC4 << ").\n";
    std::cerr << "  --abs,-A load,jump    Self-extracting decruncher parameters for absolute address location.\n";
    std::cerr << "  --exe,-X              Self-extracting decruncher (Amiga target).\n";
    std::cerr << "  --debug,-d            Output a LOT OF debug prints to stderr.\n";
    std::cerr << "  --DEBUG,-D            Output EVEN MORE debug prints to stderr.\n";
    std::cerr << "  --verbose,-v          Output some additional information to stdout.\n";
    std::cerr << "  --help,-h             Print this output ;)\n";
    std::cerr << std::flush;
    exit(EXIT_FAILURE); 
}


struct target;
typedef int (*func_t)(std::ofstream& ofs, target& trg, ...);

struct target {
    const char* target_name;
    bool is_ascii;
    int max_file_size;
    func_t init_add_header;
    func_t post_header_fix;
    func_t post_trailer_fix;
} const targets[] = {
    {   "asc",
        true,
        1<<24,
        NULL,
        NULL,
        NULL
    },
    {   "bin",
        false,
        1<<24,
        NULL,
        NULL,
        NULL
    },
    {   "zx",
        false,
        1<<16,
        NULL,
        NULL,
        NULL
    },
    {   "bbc",
        false,
        1<<16,
        NULL,
        NULL,
        NULL
    },
    {   "ami",
        false,
        1<<24,
        NULL,
        NULL,
        NULL
    }   
};


lz_config algos[] {
    {   ZXPAC4_WINDOW_MAX,  DEF_CHAIN, ZXPAC4_MATCH_MIN, ZXPAC4_MATCH_MAX, ZXPAC4_MATCH_GOOD,
        DEF_BACKWARD_STEPS, ZXPAC4_OFFSET_MATCH2_THRESHOLD, ZXPAC4_OFFSET_MATCH3_THRESHOLD,
        ZXPAC4_INIT_PMR_OFFSET,
        false,      // only_better_matches
        false,      // is_ascii
        false       // reversed_file
    },
    {   ZXPAC4B_WINDOW_MAX,  DEF_CHAIN, ZXPAC4B_MATCH_MIN, ZXPAC4B_MATCH_MAX, ZXPAC4B_MATCH_GOOD,
        DEF_BACKWARD_STEPS, ZXPAC4B_OFFSET_MATCH2_THRESHOLD, ZXPAC4B_OFFSET_MATCH3_THRESHOLD,
        ZXPAC4B_INIT_PMR_OFFSET,
        false,      // only_better_matches
        false,      // is_ascii
        false       // reversed_file
    }
};

#define LZ_TARGET_SIZE static_cast<int>((sizeof(targets)/sizeof(target)))
#define LZ_ALGO_SIZE static_cast<int>((sizeof(algos)/sizeof(lz_config)))

/**
 * @brief Driver function for a generic LZ compression..
 *
 *
 */
int handle_file(lz_base* lz, std::ifstream& ifs, std::ofstream& ofs, int len)
{
    (void)ofs;

    // extra 2 characters to avoid buffer overrun with 3 byte hash function..
    char* buf = new (std::nothrow) char[len+2];
    int n = 0;

    if (buf == NULL) {
        std::cerr << "**Error: failed to allocate memory for the input file\n";
        return -1;
    }
    if (!ifs.read(buf,len)) {
        std::cerr << "**Error: reading the input file failed\n";
        delete[] buf;
        return -1;
    }
    if (lz->lz_get_config()->is_ascii) {
        if (lz->verbose()) {
            std::cout << "Checking for ASCII only content" << std::endl;
        }

        for (n = 0; n < len; n++) {
            if (buf[n] < 0) {
                std::cerr << "**Error: ASCII only file contains bytes greater than 127\n";
                delete[] buf;
                return -1;
            }
        }
    }
    if (lz->lz_get_config()->reversed_file) {
        if (lz->verbose()) {
            std::cout << "Reversing the file for backward decompression" << std::endl;
        }

        for (n = 0; n < (len-n-1); n++) {
            char t = buf[n];
            buf[n] = buf[len-n-1];
            buf[len-n-1] = t;
        }
    }

    lz->lz_search_matches(buf,len,0); 
    lz->lz_parse(buf,len,0); 
    n = lz->lz_encode(buf,len,ofs); 

    delete[] buf;
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
    bool cfg_verbose_on = false;
    std::string cfg_infile_name;
    std::string cfg_outfile_name(DEF_OUTPUT_NAME);
    int cfg_algo = ZXPAC4;
    int cfg_good_match = -1;
    int cfg_backward_steps = -1;
    int cfg_initial_pmr_offset = -1;
    int cfg_max_chain = -1;
    bool cfg_only_better_matches = false;
    bool cfg_reversed_file = false;
    lz_base* lz = NULL;

    target trg;

    // Check target..

    for (n = 0; n < LZ_TARGET_SIZE; n++) {
        if (!strcmp(argv[1],targets[n].target_name)) {
            trg = targets[n];
            break;
        }
    }

    if (n == LZ_TARGET_SIZE) {
        usage(argv[0]);
    }

    optind = 2;

    // 
	while ((n = getopt_long(argc, argv, "g:c:e:B:i:s:p:hvdDa:X:Arb:", longopts, NULL)) != -1) {
		switch (n) {
            case 'h':
                usage(argv[0]);
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
                    std::cerr << "**Error: Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0]);
                }
                break;
            case 'r':   // --reverse
                cfg_reversed_file = true;
                break;
            case 'b':   // --only-better
                cfg_only_better_matches = true;
                break;
            case 'p':   // --pmr-offset
                cfg_initial_pmr_offset = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg_initial_pmr_offset > 127 || cfg_initial_pmr_offset < 1) {
                    std::cerr << "**Error: Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0]);
                }
                break;
            case 'c':   // --max-chain
                cfg_max_chain = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg_max_chain > MAX_CHAIN || cfg_max_chain < 1) {
                    std::cerr << "**Error: Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0]);
                }
                break;
            case 'g':   // --good-match
                cfg_good_match = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0') {
                    std::cerr << "**Error: Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0]);
                }
                break;
            case 'B':   // --backsteps
                cfg_backward_steps = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg_backward_steps > MAX_BACKWARD_STEPS || cfg_backward_steps < 0) {
                    std::cerr << "**Error: Invalid parameter value '" << optarg << "'\n";
                    usage(argv[0]);
                }
                break;
            case 'A':
                //n = sscanf(optarg,"%x,%x,%x",&load_addr,&jump_addr,&page);

                if (n != 3) {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                /*
                if (load_addr > 0xffff ||
                    jump_addr > 0xffff ||
                    page > 0xff) {
                    //fprintf(stderr,"--exe parameters out of range.\n");
                    exit(EXIT_FAILURE);;
                }
                */

                //printf("%04x, %04x, %02x\n",load_addr,jump_addr,page);
                //exe = true;
                break;
			case 'X':
                break;
            case '?':
			case ':':
				usage(argv[0]);
				exit(EXIT_FAILURE);
			default:
				break;
		}
	}

	if (argc - optind < 1) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

    // handle lz_config
    lz_config cfg = algos[cfg_algo];
    
    if (cfg_good_match > -1) {
        if (cfg_good_match > cfg.max_match) {
            std::cerr << "**Error: Invalid parameter value '" << cfg_good_match  << "'\n";
            usage(argv[0]);
        }
        cfg.good_match = cfg_good_match;
    }
    if (cfg_backward_steps > -1) {
        cfg.backward_steps = cfg_backward_steps;
    }
    if (cfg_initial_pmr_offset > -1) {
        cfg.initial_pmr_offset = cfg_initial_pmr_offset;
    }
    if (cfg_max_chain > -1) {
        cfg.max_chain = cfg_max_chain;
    }
    cfg.only_better_matches = cfg_only_better_matches;
    cfg.reversed_file = cfg_reversed_file;
    cfg.is_ascii = trg.is_ascii;

    // The following stuff is horrible.. Needs to be hidden under
    // multiple functions to avoid recurring cleanup due initialization
    // failures..

    cfg_infile_name  = std::string(argv[optind++]);

    if (argc - optind >= 1) {
        cfg_outfile_name = std::string(argv[optind++]);
    }

    ifs.open(cfg_infile_name,std::ios::binary|std::ios::in|std::ios::ate);
    if (ifs.is_open() == false) {
        std::cerr << "**Error: failed to open input file '" << cfg_infile_name << "'\n";
        goto error_exit;
    }

    file_len = ifs.tellg();
    ifs.seekg(0);

    if (file_len < 0) {
        std::cerr << "**Error: getting file length  of '" << cfg_infile_name 
            << "' failed" << std::endl;
        goto error_exit;
    }
    if (file_len > trg.max_file_size) {
        std::cerr << "**Error: maximum file length is " << trg.max_file_size
            << " bytes" << std::endl;
        goto error_exit;
    }

    try {
        switch (cfg_algo) {
        case ZXPAC4B:
            lz = new zxpac4b(&cfg);
            break;
        case ZXPAC4:
        default:
            lz = new zxpac4(&cfg);
            break;
        }
    } catch (std::exception& e) {
        std::cerr << "**Error1: " << e.what() << "\n";
        lz = NULL;
        goto error_exit;
    }
    try {
        lz->lz_cost_array_get(file_len);
    } catch (std::exception& e) {
        std::cerr << "**Error2: " << e.what() << "\n";
        goto error_exit;
    }
   
    lz->set_debug_level(cfg_debug_level);
    lz->enable_verbose(cfg_verbose_on);
    
    if (cfg_verbose_on) {
        std::cout << "Loading from file '" << cfg_infile_name << "'\n";
        std::cout << "Saving to file '" << cfg_outfile_name << "'\n";
        std::cout << "Using target '" << trg.target_name << "' and algorith " << cfg_algo << "\n";
    }

    ofs.open(cfg_outfile_name,std::ios::binary|std::ios::out);
    if (ofs.is_open()) {
        compressed_len = handle_file(lz,ifs,ofs,file_len);
        
        if (compressed_len < 0) {
            std::cerr << "**Error: compression failed\n";
            goto error_exit;
        }

        double gain = 1.0 - static_cast<double>(compressed_len) / static_cast<double>(file_len);
        std::cout << "Original: " << file_len << ", compressed: " << std::setprecision(4)
                  << compressed_len << ", gained: " << gain*100 << "%\n";

        if (cfg_verbose_on) {
            std::cout << "Number of literals: " << lz->get_num_literals() << std::endl;
            std::cout << "Number of matches: " << lz->get_num_matches() << std::endl;
            std::cout << "Number of matched bytes: " << lz->get_num_matched_bytes() << std::endl;
            std::cout << "Number of PMR matches: " << lz->get_num_pmr_matches() << std::endl; 
            std::cout << "Number of PMR literals: " << lz->get_num_pmr_literals() << std::endl;
        }


    } else {
        std::cerr << "**Error: opening output file '" << cfg_outfile_name << "' failed\n";
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
