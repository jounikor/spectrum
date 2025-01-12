/**
 * @file hunk.cpp
 * @brief Amiga executable hunk parsing and processing.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 *
 * @copyright The Unlicense
 *
 */

#include <map>
#include <iterator>
#include <fstream>
#include <algorithm>
#include "hunk.h"
#include "lz_util.h"

using namespace amiga_hunks;

static const char* s_hunk_str[] = {
    "HUNK_UNIT",	            // 0x3E7       // error
    "HUNK_NAME",                // 0x3E8       // exe remove
    "HUNK_CODE",	            // 0x3E9       // exe -> 00
    "HUNK_DATA",	            // 0x3EA       // exe -> 01
    "HUNK_BSS",	                // 0x3EB       // exe -> 10
    "HUNK_RELOC32",	            // 0x3EC       // exe -> 11
    "HUNK_RELOC16",	            // 0x3ED       // error
    "HUNK_RELOC8",	            // 0x3EE       // error
    "HUNK_EXT",		            // 0x3EF       // error
    "HUNK_SYMBOL",		        // 0x3F0       // exe remove
    "HUNK_DEBUG",		        // 0x3F1       // exe remove
    "HUNK_END",		            // 0x3F2       // exe
    "HUNK_HEADER",		        // 0x3F3       // exe
    "Unknown_hunk",
    "HUNK_OVERLAY",	            // 0x3F5       // output exe / error
    "HUNK_BREAK",		        // 0x3F6       // output exe / error
    "HUNK_DREL32 or HUNK_RELOC32SHORT_OLD",		        // 0x3F7       // error
    "HUNK_DREL16",		        // 0x3F8       // error
    "HUNK_DREL8",		        // 0x3F9       // error
    "HUNK_LIB",		            // 0x3FA       // error
    "HUNK_INDEX",		        // 0x3FB       // error
    "HUNK_RELOC32SHORT",	    // 0x3FC       // exe -> 11
    "HUNK_RELRELOC32",	        // 0x3FD       // exe
    "HUNK_ABSRELOC16"	        // 0x3FE       // error
};

static const char* s_memtype_str[] = {
    // MEMF_PUBLIC == 1
    // MEMF_CHIP   == 2
    // MEMF_FAST   == 4}
    "MEMF_ANY",
    "MEMF_PUBLIC",
    "MEMF_CHIP",
    "??",
    "MEMF_FAST",
    "??",
    "??",
};

static char* compress_relocs(char* dst, std::map<uint32_t,std::set<uint32_t> >& new_relocs, bool debug=false); 

//
//
//

uint32_t amiga_hunks::read32be(char*& ptr, bool inc)
{
    uint32_t r;
    const uint8_t* p = reinterpret_cast<uint8_t*>(ptr);
    
    r  = *p++; r <<= 8;
    r |= *p++; r <<= 8;
    r |= *p++; r <<= 8;
    r |= *p;
   
    if (inc) {
        ptr += 4;
    }
    return r;
}

uint32_t amiga_hunks::readbe(char*& ptr, int bytes, bool inc)
{
    uint32_t r = 0;
    const uint8_t* p = reinterpret_cast<uint8_t*>(ptr);
    
    if (inc) {
        ptr += bytes;
    }
    
    while (bytes-- > 0) {
        r <<= 8;
        r |= (*p++ & 0xff);
    }
    return r;
}

uint16_t amiga_hunks::read16be(char*& ptr, bool inc)
{
    uint16_t r;
    const uint8_t* p = reinterpret_cast<uint8_t*>(ptr);
    r  = *p++; r <<= 8;
    r |= *p;

    if (inc) {
        ptr += 2;
    }
    return r;
}

char* amiga_hunks::write24be(char* ptr, uint32_t r, bool inc)
{
    uint8_t* p = reinterpret_cast<uint8_t*>(ptr);
    *p++ = r >> 16;
    *p++ = r >> 8;
    *p = r;
    
    if (inc) {
        ptr += 3;
    }
    return ptr;
}

char* amiga_hunks::write32be(char* ptr, uint32_t r, bool inc)
{
    uint8_t* p = reinterpret_cast<uint8_t*>(ptr);
    *p++ = r >> 24;
    *p++ = r >> 16;
    *p++ = r >> 8;
    *p = r;
    
    if (inc) {
        ptr += 4;
    }
    return ptr;
}

char* amiga_hunks::write16be(char* ptr, uint16_t r, bool inc)
{
    uint8_t* p = reinterpret_cast<uint8_t*>(ptr);
    *p++ = r >> 8;
    *p = r;

    if (inc) {
        ptr += 2;
    }

    return ptr;
}

/**
 *
 *
 * @return The number of found hunks or <= 0 if an error.
 */

int amiga_hunks::parse_hunks(char* ptr, int size, std::vector<hunk_info_t>& hunk_list, bool debug)
{
    char* end = ptr+size;
    uint32_t n, m, j;
    uint32_t tsize, tnum, tmax;         // Follow DOS RKRM ;)
    uint32_t msj, mtj;
    uint32_t num_residents;              // archaic stuff
    uint32_t hunk_type;
    int hunk_size;
    int padding;
    bool overlay = false;

    hunk_type = read32be(ptr);
    
    TDEBUG(std::cerr << ">- Executable file Hunk Parser -------------------------" << std::endl;)
    
    if (size % 4) {
        TDEBUG(std::cerr << ERR_PREAMBLE << "Amiga executable size must be 4 byte aligned." << std::endl;)
        return -1;
    }
    if (size < 6*4 || (hunk_type != HUNK_HEADER)) { 
        TDEBUG(std::cerr << ERR_PREAMBLE << "not Amiga executable, no HUNK_HEADER found." << std::endl;)
        return -1;
    }
    TDEBUG(std::cerr << "HUNK_HEADER (0x" << std::hex << hunk_type << ")" << std::endl;)

    num_residents = 0;
    while ((n = read32be(ptr))) {
        ptr += (n * 4);
        ++num_residents;
    }

    if (num_residents > 0) {
        TDEBUG(std::cerr << "  File has " << num_residents << " resident libraries.." << std::endl;)
    }

    tsize = read32be(ptr);
    tnum  = read32be(ptr);
    tmax  = read32be(ptr);
   
    TDEBUG(                                                             \
        std::cerr << std::hex << "  Number of segments: " << tsize << std::endl;    \
        std::cerr << "  First segment:      " << tnum << std::endl;     \
        std::cerr << "  Last of segment:    " << tmax << std::endl;     \
    )
    if (num_residents != tnum) {
        std::cerr << "  First root hunk (" << tnum << ") does not match the number of residents" << std::endl;
        return -1;
    }
    if (tsize - 1 > tmax) {
        TDEBUG(std::cerr << "  Likely an OVERLAY file" << std::endl;)
    }

    hunk_list.reserve(tsize);

    for (n = tnum, m = 0; n <= tmax; n++, m++) {
        hunk_info_t hunk;
        hunk.hunks_remaining = tmax-tnum-m;
        hunk.old_seg_num = n;
        hunk.new_seg_num = -1;
        hunk.seg_start = NULL;
        hunk.reloc_start = NULL;
        hunk.merged_start_index = 0;

        msj = read32be(ptr);
        // MEMF_ANY    == 0
        // MEMF_PUBLIC == 1
        // MEMF_CHIP   == 2
        // MEMF_FAST   == 4
        // msj bit 31  == FAST
        // msj bit 30  == CHIP
        mtj = msj & 0xc0000000;
        msj = (msj & 0x3fffffff) << 2;
        hunk.combined_type = mtj;
        mtj >>= 29;

        if (mtj == (3 << 1)) {
            mtj = read32be(ptr);
        }
        TDEBUG ( std::cerr << std::hex << "  Segment: 0x" << n << ", memory size: 0x" << msj    \
                    << " bytes " << "(" << s_memtype_str[mtj] << ")" << std::endl;          \
        )

        hunk.memory_size = msj;
        hunk.memory_type = mtj;
        hunk_list.push_back(hunk);
    }

    hunk_size = 0;
    n = -1;

    // The algorithm for the hunk parser:
    //  1) the parsed result is placed into hunk_list_t vector.
    //  2) HUNK_SYMBOL/_DEBUG/_NAME are removed.
    //  3) Hunk advisory is removed.
    //  4) Both 16 bit and 32 bit relocations are supported.
    //     - All relocs are stored into map indexed by the destination segment.
    //     - All reloc to the same destination segment under the same 
    //       relcation withint one hunk are merged.
    //     - Reloc entries are sorted into ascendin order (a property of std::set).

    while (ptr < end) {
        hunk_size = 0;
        msj = read32be(ptr);
        // Bits 31 and 30 are unused
        // Bit 29 contains flag for possible hunk advisory..
        hunk_type = msj & 0x1fffffff;
        
        if (hunk_type <= LAST_SUPPORTED_HUNK) {
            TDEBUG(std::cerr << s_hunk_str[hunk_type-FIRST_SUPPORTED_HUNK] << "(0x" \
                << std::hex << hunk_type << ")" << std::endl;)
        }
        // skip memory type but parse advisory hunk flag
        if (msj & 0x20000000) {
            hunk_size = read32be(ptr);

            TDEBUG(std::cerr << "  Advisory hunk flag seen.. skipping "             \
                << hunk_size    << " long words in hunk " << tnum << std::endl;)
            ptr += hunk_size;
        }

        switch (hunk_type) {
        case HUNK_SYMBOL:
            TDEBUG(std::cerr << "  Removing.." << std::endl;)
            do {
                hunk_size = read32be(ptr) & 0x00ffffff;
                if (hunk_size == 0) {
                    break;
                }
                ptr += ((hunk_size + 1) * 4);
            } while (true);
            break;
        case HUNK_DEBUG:
        case HUNK_NAME:
            TDEBUG(std::cerr << "  Removing.." << std::endl;)
            hunk_size = read32be(ptr);
            break;
        case HUNK_DATA:
        case HUNK_CODE:
            hunk_size = read32be(ptr);
        case HUNK_BSS:
            // we increment to next segment due exe files that may not
            // contain HUNK_ENDs
            ++n;
            hunk_list[n].seg_start = ptr;
            hunk_list[n].data_size = hunk_size << 2;
            hunk_list[n].hunk_type = hunk_type;
            hunk_list[n].combined_type |= hunk_type;
            TDEBUG(std::cerr << std::dec << "  segment: " << hunk_list[n].old_seg_num   \
                    << std::hex << ", data size: 0x" << hunk_list[n].data_size          \
                    << ", memory size: 0x" << hunk_list[n].memory_size                  \
                    << ", remaining: 0x" << hunk_list[n].hunks_remaining                \
                    << ", pointer: 0x" << (uint64_t)ptr << std::endl;                   \
            )
            break;
        case HUNK_END:
            hunk_size = 0;
            break;
        case HUNK_RELOC32:
        case HUNK_RELRELOC32:
            hunk_list[n].reloc_start = ptr;
            hunk_size = 0;
            
            do {
                m = read32be(ptr);
                if (m == 0) {
                    break;
                }
                
                j = read32be(ptr);
                TDEBUG(std::cerr << std::dec << "  " << m << " relocs to segment " << j << std::endl;)
                
                // Since destination segmentt are using map a reoccurrance of relocations into an
                // existing segment are all combined into one..
                std::set<uint32_t>& s = hunk_list[n].relocs[j];

                while (m-- > 0) {
                    // Since this is a set duplicates are eliminated automatically.
                    s.insert(read32be(ptr));
                }
            } while (true);

            break;
        case HUNK_DREL32:
        case HUNK_RELOC32SHORT:
            padding = 1;
            hunk_list[n].reloc_start = ptr;
            hunk_size = 0;
            
            do {
                m = read16be(ptr);
                if (m == 0) {
                    break;
                }
            
                j = read16be(ptr);
                TDEBUG(std::cerr << std::dec << "  " << m << " relocs to segment " << j << std::endl;)
                padding += (m * 2);
                std::set<uint32_t>& s = hunk_list[n].relocs[j];
                
                while (m-- > 0) {
                    s.insert(read16be(ptr));
                }
            } while (true);
            
            if (padding & 1) {
                ++ptr;
            }

            break;
        case HUNK_BREAK:
            break;
        case HUNK_OVERLAY:
            overlay = true;
            // Definitely not sure if this is enough to parse overlay files..
            hunk_size = read32be(ptr) + 1;
            break;
        default:
            std::cerr << ERR_PREAMBLE << "unsupported hunk 0x" << std::hex << hunk_type << std::endl;
            return -1;
        }

        ptr += (hunk_size * 4);
    }

    if (++n < tsize) {
        TDEBUG(std::cerr << "Number of parsed segments does not match with the header information.." << std::endl;)
    }
    if (n > MAX_SEGMENT_NUM) {
        std::cerr << ERR_PREAMBLE << "too many segments (" << n << ")\n";
        return -1;
    }
    if (hunk_list[0].data_size < 8) {
        // We need at least 8 bytes of data size in the first hunk to host the
        // decompressor tramboline code..
        std::cerr << ERR_PREAMBLE << "the first huml must be at least 8 bytes\n";
        return -1;
    }
    if (overlay) {
        return -1;
    } else {
        return n;
    }
}


/**
 * @brief Optimized AmigaDOS executable file hunk format to a slightly
 *        more compact form. 
 *
 * 
 * @return The length of the output file returned in @p new_exe.
 *         Zero or negative in case of an error.
 */
int amiga_hunks::optimize_hunks(char* exe, int len, const std::vector<hunk_info_t>& hunk_list,
                                char*& new_exe, std::vector<uint32_t>* new_segments,bool debug)
{
    (void)exe;
    (void)debug;

    std::map<uint32_t,std::set<uint32_t>> new_relocs;
    char* ptr;
    int m,n;
    uint32_t seg_num = hunk_list.size(); 
    
    new_exe = new char[len];
    ptr = new_exe;

    // For the HUNK_HEADER but not part of the to be compressed new
    // executable, which has a proprietary format.
    //
    // The exported new_segments vector has the following data:
    //  [0] hunk memroy size in long words and memory type as in legacy
    //  [0] legacy hunk type
    //  [0] hunk data size in long words or in some cases the seek position..
    //  ...
    //  [n] hunk memroy size in long words and memory type as in legacy
    //  [n] legacy hunk type
    //  [n] hunk data size in long words or in some cases the seek postioon..
    //
    for (seg_num = 0; seg_num < hunk_list.size(); seg_num++) {
        new_segments->push_back((hunk_list[seg_num].memory_size >> 2) | 
                            (hunk_list[seg_num].combined_type & 0xc0000000));
        new_segments->push_back(hunk_list[seg_num].hunk_type);
        new_segments->push_back(hunk_list[seg_num].data_size >> 2);
    }

    // Output each new segment
    for (seg_num = 0; seg_num < hunk_list.size(); seg_num++) {
        bool move_data = true;
        // output segment memory type and data size in 32bit long words
        switch (hunk_list[seg_num].combined_type & 0x3fffffff) {
            case HUNK_CODE:
                ptr = write32be(ptr,(hunk_list[seg_num].data_size >> 2) | SEGMENT_TYPE_CODE);
                break;
            case HUNK_DATA:
                ptr = write32be(ptr,(hunk_list[seg_num].data_size >> 2) | SEGMENT_TYPE_DATA);
                break;
            case HUNK_BSS:
                ptr = write16be(ptr,SEGMENT_TYPE_BSS);
                move_data = false;
                break;
            default:
                assert("Unknown hunk type" == NULL);
                break;
        }
       
        if (hunk_list[seg_num].reloc_start) {
            for (auto& reloc : hunk_list[seg_num].relocs) {
                uint32_t key = (seg_num << 16) | reloc.first;
                std::set<uint32_t>& entries = new_relocs[key];
                
                for (auto& entry : reloc.second) {
                    entries.insert(entry);
        }   }   }
        if (move_data) {
            m = hunk_list[seg_num].data_size;
            n = 0;

            while (m-- > 0) {
                *ptr++ = hunk_list[seg_num].seg_start[n++];
    }   }   }

    // Compress relocation information..
    ptr = compress_relocs(ptr,new_relocs,debug);
    ptr = write16be(ptr,SEGMENT_TYPE_EOF);
    
    // done.. 
    n = (ptr-new_exe);
    TDEBUG(std::cerr << ">> New exe size after reloc data: 0x" << n << std::endl;)
    TDEBUG(                                                                         \
        for (auto& aa : new_relocs) {                                               \
            uint32_t rel_info = aa.first;                                           \
            std::cerr << std::hex << "** new relocs 0x" << rel_info << "\n";        \
            for (auto& bb : aa.second) {                                            \
                std::cerr << "   0x" << bb << "\n";                                 \
            }                                                                       \
    })

    return n;
}




/**
 *
 * @return 0 if no changes and no @p new_exe was allocated
 *         -1 if an error occured
 *         > 0 new number of hunks in $p hunk_list
 *
 *
 *
 */
int amiga_hunks::merge_hunks(char* exe, int len, std::vector<hunk_info_t>& hunk_list,
                            char*& new_exe, std::vector<uint32_t>* new_segments, bool debug)
{
    (void)len;

    int o, p, m, n;
    int num_old_seg = hunk_list.size();
    std::map<uint32_t,int> new_hunks;
    std::map<uint32_t,std::set<uint32_t>> new_relocs;
    int num_new_seg = 0;
    new_exe = new char[len];
    char* ptr = new_exe;

    // Oh bummer.. I do not check if the memory allocations actually fail or
    // not in the following temporary workspace allocations. Shame on me.

    // A list of original executable file hubk/segment code/data sizes.
    // The entries do not contain the possible associated BSS. The list
    // is indexed using the original executable file hunk/segment numbers.
    // The size of the list is the number of the original executable file
    // hunk/segments.

    uint32_t* merged_old_seg_size = new (std::nothrow) uint32_t[num_old_seg];
    
    // A list of the original executable file hubk/segment BSS sizes. Note,
    // not all code/data hunks contain an associated BSS.  The list
    // is indexed using the original executable file hunk/segment numbers.
    // The size of the list is the number of the original executable file
    // hunk/segments.

    int* old_seg_bss = new (std::nothrow) int[num_old_seg];
    
    // A list of original executable file hunk offsets after all hunks have been
    // merged. The offsets are within the merged executable file but indexed using
    // the original hunk/segment numbers. The size of the list is the number of
    // segments in the original executable file.

    int* merged_old_seg_offs = new (std::nothrow) int[num_old_seg];

    // This list is used with merged_old_seg_offs and indexes similarly using the 
    // original executable file hunk/segment numbers. If a code/data hunk had an
    // assiciated BSS area, then this list contains the adjustment offset for each
    // old hunk/segment in the new merged hunk how much the relocation entry in 
    // the file has to be offsetted to be inthe BSS area. The below drawing attemps
    // to illustrate what this means:
    //
    // Original HUNK_DATA #1 with an associated BSS
    //
    // +-----------------+--------+
    // | HUNK_DATA #1    | BSS #1 | 
    // +-----------------+--------+
    //
    // Original HUNK_DATA #2 with an associated BSS
    //
    // +--------------+--------------+
    // | HUNK_DATA #3 | BSS #3       | 
    // +--------------+--------------+
    //
    // |<- Merged HUNK x ---------------|- merged hunk x BSS -->|
    // +-----------------+--------------+--------+--------------+
    // | HUNK_DATA #1    | HUNK_DATA #3 | BSS #1 | BSS #3       |
    // +-----------------+--------------+--------+--------------+
    // |<- n bytes ----->|
    //                   |<- m bytes -->|
    //                                  |<- N -->|
    //                                           |<- M bytes -->|
    // |<- offset n=0
    // |-- offset m=n -->|
    // |-- bss n offset --------------->|
    // |-- bss m offset ------------------------>|
    //
    // merged_old_seg_offs[1] = 0;
    // merged_old_seg_offs[3] = n;
    // merged_old_seg_bss_offs[1] = n+m;
    // merged_old_seg_bss_offs[3] = n+m+N;

    int* merged_old_seg_bss_offs = new (std::nothrow) int[num_old_seg];

    // The hunk merging algorithms works as follows:
    //  1) Combine hunks with the same type and the memory requirement into a single merged hunk.
    //  2) If merged hunks (HUNK_DATA and HUNK_CODE) have attaced BSS section those are also
    //     included into the merged hunk. There can be total 9 merged hunks.
    //  3) CODE/DATA/BSS hunks are separated from relocation information. The relocation information
    //     is stored as a one binary blob at the end of all merged hunks.
    //  4) Relocation datas are also merged accordingly to merged hunks. Duplicates are removed and
    //     possible multiple relocations to the same hunk are combined.
    //  5) Relocation entries are delta encoded.
    //
    //  Note that there are some restrictions.
    //  - Maximum number of merged hunks is 9 but if the encoder does not not merge hunks then
    //    the limit is 65534. The value 65535 is reserved for an EOF marker.
    //  - Maximum length of a reloc entry is 2^24-1.
    
    for (std::vector<hunk_info_t>::iterator it = hunk_list.begin(), et = hunk_list.end(); it != et; it++) {
        if (new_hunks.find(it->combined_type) == new_hunks.end()) {
            new_hunks[it->combined_type] = num_new_seg++;
        }
        it->new_seg_num = new_hunks[it->combined_type];
        merged_old_seg_size[it->old_seg_num] = it->data_size;
        old_seg_bss[it->old_seg_num] = it->memory_size - it->data_size;
    }

    // Explanation of the following assistance tables used for lookups during hunk merging:
    //
    // A list of char pointers to the start of each merged hunk. Each merged hunk contains
    // one or more original hunks of the same type and memory allocation type.
    // The list size is the number of total merged hunks, which is less of equal to the
    // original number of hunks.
   
    char** merged_hunk_start   = new (std::nothrow) char*[num_new_seg];   

    // A list of merged hunk data sizes type of uint32_t. Each merged hunk is a sum of
    // merged original hunks. Possible BSS attached to original HUNK_DATAs are not part
    // of these sizes. The size of the list is the number of new merged hunks.
  
    uint32_t* merged_data_size = new (std::nothrow) uint32_t[num_new_seg];
   
    // A list of offsets to the start of each merged hunk and type of uint32_t. The 
    // first offset is always zero i.e. the beginning of the merged binary blob. The
    // size of the list is the number of merged hunks plus one (the last list entry
    // is not every used except during the calculation of the offsets).
 
    uint32_t* merged_data_offs = new (std::nothrow) uint32_t[num_new_seg+1];
    merged_data_offs[0] = 0;
    
    // A list of merged hunk types. Each entry contains the memory type in bits 31 & 30,
    // and the standard AmigaDOS hunk type in bits 29-0. The size of the list is the 
    // number of new merged hunks.
    
    uint32_t* merged_hunk_type = new (std::nothrow) uint32_t[num_new_seg];
    
    // A list of merged hunk memory sized, which also include possible associated
    // BSS hunk. It is possible to have BSS associeated with HUNK_CODE and HUNK_DATA.
    // The size of the list is the number of new merged hunks.

    uint32_t* merged_mem_size  = new uint32_t[num_new_seg];
    
    // A list of merged hunk BSS sizes. Note that a merged code or data hunk may
    // also contain a BSS. The size of the list is the number of new merged hunks.

    uint32_t* merged_bss_size  = new (std::nothrow) uint32_t[num_new_seg];
    
    // We hav a memory allocation issue..? Kinda useless but for completeness ;)
    if (!(merged_hunk_start && merged_data_size && merged_data_offs && merged_hunk_type &&
          merged_mem_size && merged_bss_size && merged_old_seg_size && merged_old_seg_offs &&
          old_seg_bss && merged_old_seg_bss_offs)) {
    
        std::cerr << ERR_PREAMBLE << "allocation of work space failed" << std::endl;
        n = -1;
        goto alloc_error;
    }

    for (m = 0; m < num_new_seg; m++) {
        merged_data_size[m] = 0;
        merged_mem_size[m] = 0;
        merged_bss_size[m] = 0;
        merged_old_seg_offs[m] = 0;
    }
    for (m = 0; m < num_old_seg; m++) {
        merged_old_seg_offs[m] = merged_data_size[new_hunks[hunk_list[m].combined_type]]; 
        merged_old_seg_size[m] = hunk_list[m].data_size;
        merged_hunk_type[hunk_list[m].new_seg_num] = hunk_list[m].combined_type;
        merged_data_size[hunk_list[m].new_seg_num] += hunk_list[m].data_size;
        merged_mem_size[hunk_list[m].new_seg_num] += hunk_list[m].memory_size;
    }
    for (m = 0; m < num_old_seg; m++) {
        merged_old_seg_bss_offs[m] = merged_data_size[hunk_list[m].new_seg_num] + merged_bss_size[hunk_list[m].new_seg_num];
        merged_bss_size[hunk_list[m].new_seg_num] += old_seg_bss[m];
        TDEBUG(std::cerr << "Binnary pointers: " << reinterpret_cast<uint64_t>(hunk_list[m].seg_start) << "\n";)
    }

    TDEBUG(std::cerr << "Number of merged hunks is " << num_new_seg << std::endl;)
    
    for (m = 0; m < num_new_seg; m++) {
        merged_data_offs[m+1] = merged_data_offs[m] + merged_data_size[m];

        TDEBUG( \
            std::cerr << "Merged new segment " << m << ":" << std::endl;                        \
            std::cerr << "  Hunk type -> " << merged_hunk_type[m] << std::endl;                 \
            std::cerr << "  Sizes  -> " << merged_data_size[m] << std::endl;                    \
            std::cerr << "  Offset from beginning -> " << merged_data_offs[m] << std::endl;)
    }
    TDEBUG(for (m = 0; m < num_old_seg; m++) {                                                  \
        std::cerr << "Old segment " << m << ":" << std::endl;                                   \
        std::cerr << "  Mapped to -> " << hunk_list[m].new_seg_num << std::endl;                \
        std::cerr << "  Offset -> " << merged_old_seg_offs[m] << std::endl;                     \
        std::cerr << "  Size -> " << merged_old_seg_size[m] << std::endl;                              \
        std::cerr << "  BSS -> " << old_seg_bss[m] << std::endl;                                \
        std::cerr << "  BSS offset " << merged_old_seg_bss_offs[m] << std::endl; }                     \
    
        for (m = 0; m < num_new_seg; m++) {                                                     \
            std::cerr << "Merged segment " << m << " end  -> " << m << ": " << merged_data_size[m]  \
                << ", " << merged_mem_size[m] << std::endl;                                     \
    })

    // Just for my own education.. Did not remember this ;)
    // https://stackoverflow.com/questions/7648756/is-the-order-of-iterating-through-stdmap-known-and-guaranteed-by-the-standard
    TDEBUG(for (auto& i: new_hunks)  {                                                          \
        std::cerr << "Combined mapping -> " << i.first << ": " << i.second << std::endl;        \
    })

    // For the HUNK_HEADER..
    for (m = 0; m < num_new_seg; m++) {
        new_segments->push_back((merged_mem_size[m] >> 2) | (merged_hunk_type[m] & 0xc0000000));
        new_segments->push_back(merged_hunk_type[m] & 0x3fffffff);
        new_segments->push_back(merged_data_size[m] >> 2);
    }

    // Output each new segment
    for (m = 0; m < num_new_seg; m++) {
        bool move_data = true;
        // output segment memory type and data size in 32bit long words
        switch (merged_hunk_type[m] & 0x3fffffff) {
            case HUNK_CODE:
                ptr = write32be(ptr,(merged_data_size[m] >> 2) | SEGMENT_TYPE_CODE);
                break;
            case HUNK_DATA:
                ptr = write32be(ptr,(merged_data_size[m] >> 2) | SEGMENT_TYPE_DATA);
                break;
            case HUNK_BSS:
                ptr = write16be(ptr,SEGMENT_TYPE_BSS);
                move_data = false;
                break;
            default:
                assert("Unknown hunk type" == NULL);
                break;
        }
        
        merged_hunk_start[m] = ptr;

        if (move_data) {
            for (n = 0; n < num_old_seg; n++) {
                if (m == hunk_list[n].new_seg_num) {
                    o = hunk_list[n].data_size;
                    p = 0;

                    TDEBUG(                                                                         \
                    std::cerr << "Binary Offset copying to: " << (uint32_t)(ptr-new_exe) << "\n";   \
                    std::cerr << "Binary ptr " << (uint64_t)hunk_list[n].seg_start << "\n";         \
                    std::cerr << "Binary Offset copying from: " << (uint32_t)(hunk_list[n].seg_start-exe) << "\n";)

                    while (o-- > 0) {
                        *ptr++ = hunk_list[n].seg_start[p++];
    }   }   }   }   }

    TDEBUG (                                                                            \
        std::cerr << ">> New exe size before reloc data: " << (ptr-new_exe) << std::endl;   \
        for (auto& hunks : hunk_list) {                                                     \
            std::cerr << std::dec << "Old segment " << hunks.old_seg_num << "(-> "          \
                << hunks.new_seg_num << ")\n";                                              \
            for (auto& rels : hunks.relocs) {                                               \
                std::cerr << std::dec << "  **reloc to hunk " << rels.first << std::endl;   \
                for (auto& p2 : rels.second) {                                              \
                    std::cerr << std::hex << "  0x" << p2 << std::endl;                     \
        }   }   })

    for (int new_seg_num = 0; new_seg_num < num_new_seg; new_seg_num++) {
        uint32_t base_offset = merged_data_offs[new_seg_num];

        TDEBUG(std::cerr << std::dec << "New Segment " << new_seg_num << std::hex       \
            << " with binary offset 0x" << base_offset <<std::endl;)

        // Merge relocs to new merged hunks. We use the property of sdt::set to
        // keep reloc entries in an ascending order.

        for (int old_seg_num = 0; old_seg_num < num_old_seg; old_seg_num++) {
            if (hunk_list[old_seg_num].new_seg_num == new_seg_num) {
                for (auto& reloc : hunk_list[old_seg_num].relocs) {
                    int old_dst_seg = reloc.first;
                    int new_dst_seg = hunk_list[old_dst_seg].new_seg_num;

                    // The intermediate hunk reloc structure is keyed by the 
                    // new_seg_num and new_dst_seg pair.. When encoding to the file
                    // both them need to be output.
                    int new_dst_key = (new_seg_num<<16)|new_dst_seg;
                    
                    std::set<uint32_t>& old_dst_relocs = reloc.second;
                    std::set<uint32_t>& new_dst_relocs = new_relocs[new_dst_key];
                
                    TDEBUG(std::cerr << std::dec << "  Mapping old dst segment " << old_dst_seg         \
                        << " relocs to new " << "dst segment " << new_dst_seg << " relocs with key 0x"  \
                        << std::hex << new_dst_key  << ", old seg size 0x"                              \
                        << merged_old_seg_size[old_dst_seg] << "\n";                                    \
                    )

                    for (auto& entry : old_dst_relocs) {
                        TDEBUG(std::cerr << "    0x" << entry << " + 0x"                                \
                            << merged_old_seg_offs[old_seg_num]                                         \
                            << " == 0x" << entry+merged_old_seg_offs[old_seg_num];                      \
                        )

                        // Insert the offsetted reloc entry into the new hunk..
                        new_dst_relocs.insert(entry+merged_old_seg_offs[old_seg_num]);  
                        
                        // Fix offset within the file
                        char* addr = merged_hunk_start[new_seg_num] + merged_old_seg_offs[old_seg_num] + entry; 
                        uint32_t offs = read32be(addr,false);

                        TDEBUG(std::cerr << ", reloc offset 0x" << offs << " + 0x"                      \
                            << merged_old_seg_offs[old_dst_seg] << "\n";)

                        //if (old_seg_num == old_dst_seg && offs >= merged_old_seg_size[old_seg_num]) {
                        if (offs >= merged_old_seg_size[old_dst_seg]) {
                            // we hit the BSS area..
                            TDEBUG(std::cerr << "    >> into BSS, offset with 0x"                       \
                                << merged_old_seg_bss_offs[old_dst_seg] << std::endl;)
                            offs += merged_old_seg_bss_offs[old_dst_seg];
                        } else {
                            TDEBUG(std::cerr << "    -- offset with 0x"                                 \
                                << merged_old_seg_offs[old_dst_seg] << std::endl;)
                            offs += merged_old_seg_offs[old_dst_seg]; 
                        }
                        
                        // Fix offset in exe binary
                        write32be(addr,offs);
    }   }   }   }   }

    // Compress relocation information..
    ptr = compress_relocs(ptr,new_relocs,debug);
    ptr = write16be(ptr,SEGMENT_TYPE_EOF);
    
    n = (ptr-new_exe);
    TDEBUG(std::cerr << ">> New exe size after reloc data: " << n << std::endl;)
    TDEBUG(                                                                         \
        for (auto& aa : new_relocs) {                                               \
            uint32_t rel_info = aa.first;                                           \
            std::cerr << std::hex << "** new relocs 0x" << rel_info << "\n";        \
            for (auto& bb : aa.second) {                                            \
                std::cerr << "   0x" << bb << "\n";                                 \
            }                                                                       \
    })

    // clean up
alloc_error:
    if (merged_hunk_start) delete[] merged_hunk_start;
    if (merged_data_size) delete[] merged_data_size;
    if (merged_data_offs) delete[] merged_data_offs;
    if (merged_hunk_type) delete[] merged_hunk_type;
    if (merged_mem_size) delete[] merged_mem_size;
    if (merged_bss_size) delete[] merged_bss_size;
    if (merged_old_seg_size) delete[] merged_old_seg_size;
    if (merged_old_seg_offs) delete[] merged_old_seg_offs;
    if (old_seg_bss) delete[] old_seg_bss;
    if (merged_old_seg_bss_offs) delete[] merged_old_seg_bss_offs;
    return n;
}


static char* compress_relocs(char* dst, std::map<uint32_t,std::set<uint32_t> >& new_relocs, bool debug)
{
    uint32_t src_seg;
    uint32_t dst_seg;
    uint32_t base;
    uint32_t delta, value;

    for (auto& reloc : new_relocs) {
        src_seg = reloc.first >> 16;
        dst_seg = reloc.first & 0xffff;
        std::set<uint32_t>::iterator it = reloc.second.begin();
        base = 0;

        assert(base <= MAX_RELOC_OFFSET); 
        dst = write16be(dst,dst_seg+1);
        dst = write16be(dst,src_seg+1);
        
        TDEBUG(std::cerr << std::dec << "Segment " << src_seg << ", destination "   \
            << dst_seg << std::hex << ", base 0x" << base << ", entries "           \
            << std::dec << reloc.second.size() << std::endl;)

        // The assumption is that entries are in ascending order..
        for (; it != reloc.second.end(); it++) {
            value = *it;
            assert(value > base);
            delta = value - base;
            TDEBUG(std::cerr << std::hex << "  delta 0x" << delta << "\n";)
            base = value;
            delta >>= 1;
       
            if (delta < 0x80) {
            } else if (delta < 0x4000) {
                *dst++ = (delta >> 7) | 0x80;
            } else if (delta < 0x200000) {
                *dst++ = (delta >> 14) | 0x80;
                *dst++ = (delta >> 7) | 0x80;
            } else if (delta < 0x10000000) {
                *dst++ = (delta >> 21) | 0x80;
                *dst++ = (delta >> 14) | 0x80;
                *dst++ = (delta >> 7) | 0x80;
            } else {
                assert (delta < 0x10000000);
            }

            *dst++ = delta & 0x7f;
        }
        // end mark
        *dst++ = 0x00;

        if (reinterpret_cast<uint64_t>(dst) & 1) {
            *dst++ = 0x00;
        }
    }

    return dst;
}
