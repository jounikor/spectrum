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
#include "cost.h"
#include "lz_util.h"
#include "lz_base.h"

// Constants for LZ stuff
//
//

#define DEF_MAX_CHAIN       16
#define DEF_TARGET          "zx"
#define DEF_ALGO            "zxpac4"
#define DEF_OUTPUT_NAME     "zx.pac"
#define DEF_BACKWARD_STEPS  0
#define MAX_BACKWARD_STEPS  16
#define MAX_CHAIN	        9999

#define DEF_GOOD_MATCH      32

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
	{"debug",       no_argument,        NULL, 'd'},
    {"target",      required_argument,  NULL, 't'},
    {"abs",         required_argument,  NULL, 'A'},
    {"exe",         no_argument,        NULL, 'X'},
    {"help",        no_argument,        NULL, 'h'},
    {0,0,0,0}
};

void usage( char *prg ) {
    std::cerr << "Usage: " << prg << " [options] infile [outfile]\n";
	std::cerr << " Options:\n";
    std::cerr << "  --max-chain,-c num    Maximum number of stored matches per position (default "
              << DEF_MAX_CHAIN << ", min 1, max " << MAX_CHAIN << ").\n";
    std::cerr << "  --good-match,-g num   match length that cuts further searches (default "
              << DEF_GOOD_MATCH << ", min " << MATCH_MIN <<", max " << MATCH_MAX << ").\n";
    std::cerr << "  --backward,-B num     Number of backward steps after a found match (default "
              << DEF_BACKWARD_STEPS << ", min 0, max " << MAX_BACKWARD_STEPS << ").\n";
    std::cerr << "  --only-better,-b      Further matches in the history must always be better than "
              << "previous matches for\n"
              << "                        the same position (default no).\n";
    std::cerr << "  --pmr-offset,-p       Initial PMR offset between 1 and 127 (default " << INIT_PMR_OFFSET << ").\n";
    std::cerr << "  --reverse,-r          Reverse the input file to allow end-to-start decompression "
              << "(default no reverse)\n";
    std::cerr << "  --ascii,-a            The input file must be 7bit ASCII (default 8bit binary)\n";
    std::cerr << "  --target,-t target    Select the target architecture. Supported are ZX Spectrum ('zx'),\n"
                 "                        Amiga ('ami'), BBC C ('bbc') and PC Engine ('pc') (default "
              << DEF_TARGET << "').\n";
    std::cerr << "  --abs,-A load,jump    Self-extracting decruncher parameters for absolute address location.\n";
    std::cerr << "  --exe,-X              Self-extracting decruncher (Amiga target).\n";
    std::cerr << "  --debug,-d            Output a LOT OF debug prints to stderr. With --verbose the\n"
                 "                        debug outout is even more.\n";
    std::cerr << "  --verbose,-v          Output some additional information to stdout.\n";
    std::cerr << "  --help,-h             Print this output ;)\n";
    std::cerr << std::flush;
    exit(EXIT_FAILURE); 
}

static bool s_debug_on = false;
static bool s_verbose_on = false;
static std::string s_infile_name;
static std::string s_outfile_name(DEF_OUTPUT_NAME);
static std::string s_target(DEF_TARGET);
// default configs.. per target default can be added.

typedef int (*func_t)(std::ofstream& ofs, ...);

#if 1
struct targets_ {
    const char* target_name;
    lz_config defaults;
    int max_file_size;
    func_t init_add_header;
    func_t post_header_fix;
    func_t post_trailer_fix;
} targets[] = {
    {   "zx",
        {   WINDOW_MAX,  DEF_MAX_CHAIN, MATCH_MIN, MATCH_MAX, MATCH_GOOD,
            DEF_BACKWARD_STEPS, OFFSET_MATCH2_THRESHOLD, OFFSET_MATCH3_THRESHOLD,
            INIT_PMR_OFFSET,
            false,      // only_better_matches
            false,      // is_ascii
            false       // reversed_file
        },
        1<<16,
        NULL,
        NULL,
        NULL
    },
    {   "bbc",
        {   WINDOW_MAX,  DEF_MAX_CHAIN, MATCH_MIN, MATCH_MAX, MATCH_GOOD,
            DEF_BACKWARD_STEPS, OFFSET_MATCH2_THRESHOLD, OFFSET_MATCH3_THRESHOLD,
            INIT_PMR_OFFSET,
            false,      // only_better_matches
            false,      // is_ascii
            false       // reversed_file
        },
        1<<16,
        NULL,
        NULL,
        NULL
    },
    {   "pce",
        {   WINDOW_MAX,  DEF_MAX_CHAIN, MATCH_MIN, MATCH_MAX, MATCH_GOOD,
            DEF_BACKWARD_STEPS, OFFSET_MATCH2_THRESHOLD, OFFSET_MATCH3_THRESHOLD,
            INIT_PMR_OFFSET,
            false,      // only_better_matches
            false,      // is_ascii
            false       // reversed_file
        },
        1<<16,
        NULL,
        NULL,
        NULL
    },
    {   "ami",
        {   WINDOW_MAX,  DEF_MAX_CHAIN, MATCH_MIN, MATCH_MAX, MATCH_GOOD,
            DEF_BACKWARD_STEPS, OFFSET_MATCH2_THRESHOLD, OFFSET_MATCH3_THRESHOLD,
            INIT_PMR_OFFSET,
            false,      // only_better_matches
            false,      // is_ascii
            false       // reversed_file
        },
        1<<24,
        NULL,
        NULL,
        NULL
    },
    {   NULL,
        {   0,0,0,0,0,0,0,0,0,
            false, false, false
        },
        0,
        NULL,
        NULL,
        NULL
    }   
};
#endif


const lz_config lz_defaults[] = {
    {   WINDOW_MAX,  
        DEF_MAX_CHAIN,
        MATCH_MIN,
        MATCH_MAX,
        MATCH_GOOD,
        DEF_BACKWARD_STEPS,
        OFFSET_MATCH2_THRESHOLD,
        OFFSET_MATCH3_THRESHOLD,
        INIT_PMR_OFFSET,
        false,      // only_better_matches
        false,      // is_ascii
        false       // reversed_file
    },
};

#define LZ_CONFIG_SIZE (sizeof(lz_defaults)/sizeof(lz_config);

/**
 * @brief Driver function for a generic LZ compression..
 *
 *
 */
template<typename L>
int handle_file(lz_base<L>* lz, std::ifstream& ifs, std::ofstream& ofs, int len)
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


    // Supported algorithms..
    zxpac4* lz = NULL;

    // Config - this should be tabled later on for defaults..
    lz_config cfg;

    cfg = lz_defaults[0];



    // 
	while ((n = getopt_long(argc, argv, "g:c:e:B:i:s:p:hvdat:X:Arb", longopts, NULL)) != -1) {
		switch (n) {
            case 'h':
                usage(argv[0]);
                break;
			case 'd':   // --debug
                s_debug_on = true;
                std::cerr << "debug" << std::endl;
				break;
			case 'v':   // --verbose
                s_verbose_on = true;
				break;
            case 't':   // --target
                if (std::strcmp(optarg,DEF_TARGET)) {
                    std::cerr << "**Error: Only supported target is '" << DEF_TARGET << "'\n";
                    usage(argv[0]);
                }
                break;
            case 'r':   // --reverse
                cfg.reversed_file = true;
                break;
            case 'a':   // --ascii
                cfg.is_ascii = true;
                break;
            case 'b':   // --only-better
                cfg.only_better_matches = true;
                break;
            case 'p':   // --pmr-offset
                cfg.initial_pmr_offset = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg.initial_pmr_offset > 127 || cfg.initial_pmr_offset < 1) {
                    std::cerr << "**Error: Invalid parameter value '" << optarg << "\n";
                    usage(argv[0]);
                }
                break;
            case 'c':   // --max-chain
                cfg.max_chain = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg.max_chain > MAX_CHAIN || cfg.max_chain < 1) {
                    std::cerr << "**Error: Invalid parameter value '" << optarg << "\n";
                    usage(argv[0]);
                }
                break;
            case 'g':   // --good-match
                cfg.good_match = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg.good_match > MATCH_MAX || cfg.good_match < MATCH_MIN) {
                    std::cerr << "**Error: Invalid parameter value '" << optarg << "\n";
                    usage(argv[0]);
                }
                break;
            case 'B':   // --backsteps
                cfg.backward_steps = std::strtoul(optarg,&endptr,10);
                if (*endptr != '\0' || cfg.backward_steps > MAX_BACKWARD_STEPS || cfg.backward_steps < 0) {
                    std::cerr << "**Error: Invalid parameter value '" << optarg << "\n";
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

    // The following stuff is horrible.. Needs to be hidden under
    // multiple functions to avoid recurring cleanup due initialization
    // failures..

    s_infile_name  = std::string(argv[optind++]);

    if (argc - optind >= 1) {
        s_outfile_name = std::string(argv[optind++]);
    }

    ifs.open(s_infile_name,std::ios::binary|std::ios::in|std::ios::ate);
    if (ifs.is_open() == false) {
        std::cerr << "**Error: failed to open input file '" << s_infile_name << "'\n";
        goto error_exit;
    }

    file_len = ifs.tellg();
    ifs.seekg(0);

    if (file_len < 0) {
        std::cerr << "**Error: getting file length  of '" << s_infile_name 
            << "' failed" << std::endl;
        goto error_exit;
    }
    if (file_len > ((1 << 24) - 1)) {
        std::cerr << "**Error: maximum file length is " << (1 << 24) - 1 
            << " bytes" << std::endl;
        goto error_exit;
    }

    try {
        lz = new zxpac4(&cfg);
        lz->enable_debug(s_debug_on);
        lz->enable_verbose(s_verbose_on);
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
    
    if (s_verbose_on) {
        std::cout << "Loading from file '" << s_infile_name << "'\n";
        std::cout << "Saving to file '" << s_outfile_name << "'\n";
    }

    ofs.open(s_outfile_name,std::ios::binary|std::ios::out);
    if (ofs.is_open()) {
        compressed_len = handle_file(lz,ifs,ofs,file_len);
        
        if (compressed_len < 0) {
            std::cerr << "**Error: compression failed\n";
            goto error_exit;
        }

        double gain = 1.0 - static_cast<double>(compressed_len) / static_cast<double>(file_len);
        std::cout << "Original: " << file_len << ", compressed: " << std::setprecision(4)
                  << compressed_len << ", gained: " << gain*100 << "%\n";

        if (s_verbose_on) {
            std::cout << "Number of literals: " << lz->get_num_literals() << std::endl;
            std::cout << "Number of matched bytes: " << lz->get_num_matched_bytes() << std::endl;
            std::cout << "Number of matches: " << lz->get_num_matches() << std::endl;
            std::cout << "Number of PMR matches: " << lz->get_num_pmr_matches() << std::endl; 
            std::cout << "Number of PMR literals: " << lz->get_num_pmr_literals() << std::endl;
        }


    } else {
        std::cerr << "**Error: opening output file '" << s_outfile_name << "' failed\n";
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
