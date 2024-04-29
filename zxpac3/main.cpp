#include <iosfwd>
#include <fstream>
#include <new>
#include <cstring>
#include <vector>


#include <getopt.h>

#include "bfhp.h"
#include "bfhp_cost.h"
#include "lz_util.h"
#include "lz_base.h"

// Constants for LZ stuff
//
//

#define DEF_MAX_CHAIN   16
#define DEF_BACKSTEPS   1
#define DEF_MTF_ESCAPES 1
#define DEF_MTF_INSERT  96
#define DEF_STEPS       1
#define DEF_PARSER      "rev"
#define DEF_TARGET      "zx"
#define DEF_ALGO        "3"


// Did not want to "reinvent" a command line parser thus
// we use the old trusty getopt..

static struct option longopts[] = {
    // string matcher parameterization
	{"max-chain",   required_argument,  NULL, 'c'},
	{"backsteps",   required_argument,  NULL, 'b'},
    // mtf256 parameterization
	{"mtf-escapes", required_argument,  NULL, 'e'},
	{"mtf-insert",  required_argument,  NULL, 'i'},
	{"mtf-steps",   required_argument,  NULL, 's'},
	{"mtf-preload", required_argument,  NULL, 'p'},
    // generix parameters
	{"verbose",     no_argument,        NULL, 'v'},
	{"debug",       no_argument,        NULL, 'd'},
	{"asm",         no_argument,        NULL, 'A'},
    {"target",      required_argument,  NULL, 't'},
    {"exe",         required_argument,  NULL, 'x'},
    {"parser",      required_argument,  NULL, 'r'},
    {"algo",        required_argument,  NULL, 'a'},
    {"help",        no_argument,        NULL, 'h'},


    {0,0,0,0}
};

void usage( char *prg ) {
    std::cerr << "Usage: " << prg << " [options] infile outfile\n";
	std::cerr << " Options:\n";
    std::cerr << "  --max-chain,-c num    Maximum number of stored matches per position (default "
              << DEF_MAX_CHAIN << ", min 1, max 256).\n";
    std::cerr << "  --backsteps,-b num    Number of back steps during parsing (default "
              << DEF_BACKSTEPS << ").\n";
    std::cerr << "  --mtf-escapes,-e num  Number of reserved MTF escape codes (defaults "
              << DEF_MTF_ESCAPES << ", min 1 and max 4).\n";
    std::cerr << "  --mtf-insert,-i pos   MTF insert position for a new code (default "
              << DEF_MTF_INSERT << ", value between 0 to 126).\n";
    std::cerr << "  --mtf-steps,-s num    Number of MTF move steps from (default "
              << DEF_STEPS << ", 0 to front, min 1, max 8).\n";
    std::cerr << "  --mtf-preload,-p file The name of the file to preload MTF context.\n";
    std::cerr << "  --target,-t target    Select the target architecture. Supported are ZX Spectrum ('zx'),\n"
                 "                        Amiga ('ami'), BBC C ('b') and PC Engine ('pc') (default "
              << DEF_TARGET << "').\n";
    std::cerr << "  --exe load,jump,page  Self-extracting decruncher parameters.\n";
    std::cerr << "  --parser,-r type      'rev' for reverse and 'fwd' for forward parsing (default '"
              << DEF_PARSER << "').\n";
    std::cerr << "  --algo,-a num         Select compression algorithm. Supported are zxpac3 (3) and\n"
              << "                        zxpac2 (2) (default " << DEF_ALGO << ").\n";
    std::cerr << "  --debug,-d            Output a LOT OF debug prints to stderr.\n";
    std::cerr << "  --verbose,-v          Output some additional information to stdout.\n";
    std::cerr << "  --asm,-A              Output ASM defines for these compression options.\n";
    std::cerr << "  --help,-h             Print this output ;)\n";
    std::cerr << std::flush;

    exit(EXIT_FAILURE); 
}

std::vector<char*> options;

static bool s_debug_on = false;
static bool s_verbose_on = false;
static bool s_asm_output = false;
static std::string s_preload_name;
static std::string s_infile_name;
static std::string s_outfile_name;
static std::string s_algo(DEF_ALGO);
static std::string s_target(DEF_TARGET);

//

template<typename L>
int handle_file(lz_base<L>* lz, cost_base* cost, std::ifstream& ifs, std::ofstream& ofs, int len)
{

    std::cout << "Onistui" << std::endl;

    return 0;
}





int main(int argc, char** argv)
{
    int n;

	while ((n = getopt_long(argc, argv, "c:b:e:i:s:p:hvdat:x:r:", longopts, NULL)) != -1) {
		switch (n) {
            case 'h':
                usage(argv[0]);
                break;
			case 'd':
                s_debug_on = true;
				break;
			case 'v':
                s_verbose_on = true;
				break;
			case 'A':   // asm output
                s_asm_output = true;
				break;
			case 'p':
                s_preload_name = std::string(optarg);
				break;
            case 't':   // target
                if (std::strcmp(optarg,DEF_TARGET)) {
                    std::cerr << "**Error: Only supported target is '" << DEF_TARGET << "'\n";
                    usage(argv[0]);
                }
                break;
            case 'a':§// algorithm
                if (std::strcmp(optarg,DEF_ALGO)) {
                    std::cerr << "**Error: Only supported algorithm is '" << DEF_ALGO << "'\n";
                    usage(argv[0]);
                }
                break;
            case 'r':   // parser
                if ((std::strcmp(optarg,"rev")) {
                    std::cerr << "**Error: Only supported parser is '" << DEF_PARSER << "'\n";
                    usage(argv[0]);
                }
                break;
            case 'e':
                //n = sscanf(optarg,"%x,%x,%x",&load_addr,&jump_addr,&page);

                if (n != 3) {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                /*
                if (load_addr > 0xffff ||
                    jump_addr > 0xffff ||
                    page > 0xff) {
                    i//fprintf(stderr,"--exe parameters out of range.\n");
                    exit(EXIT_FAILURE);;
                }
                */

                //printf("%04x, %04x, %02x\n",load_addr,jump_addr,page);
                //exe = true;
            case 'i':
                //inplace = true;
                break;
			case '?':
			case ':':
				usage(argv[0]);
				exit(EXIT_FAILURE);
			default:
				break;
		}
	}

	if (argc - optind < 2) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
 
    s_infile_name  = std::string(argv[optind++]);
    s_outfile_name = std::string(argv[optind]);

    // This is deprecatd int C++11..
    if (s_target == "") {
        s_target = DEF_TARGET;
    }
    if (s_algo == "") {
        s_algo = DEF_ALGO;
    }
    if (s_verbose_on) {
        std::cout << "Loading from file '" << s_infile_name << "'\n";
        std::cout << "Saving to file '" << s_outfile_name << "'\n";
    }

    std::ifstream ifs;
    std::ofstream ofs;
    int chain = 16;
    int len = 100;
#define NUM_ESCAPES 1

    bfhp *bf = new bfhp(65536,2,256, chain,2, 96,NUM_ESCAPES,1, 256);
    cost_base* my_cost = bf->lz_cost_array_get(len);

    handle_file(bf,my_cost,ifs,ofs);

    bf->lz_cost_array_done();
    delete bf;


    return 0;
}
