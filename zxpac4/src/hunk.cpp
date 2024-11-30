/**
 * @file hunk.cpp
 * @brief Amiga executable hunk parsing and processing.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1*
 *
 * @copyright The Unlicense
 *
 */

#include <map>
#include <iterator>
#include <fstream>
#include "hunk.h"

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



uint32_t amiga_hunks::read32be(const char*& ptr)
{
    uint32_t r;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(ptr);
    r  = *p++; r <<= 8;
    r |= *p++; r <<= 8;
    r |= *p++; r <<= 8;
    r |= *p;
    ptr += 4;
    return r;
}

uint16_t amiga_hunks::read16be(const char*& ptr)
{
    uint16_t r;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(ptr);
    r  = *p++; r <<= 8;
    r |= *p;
    ptr += 2;
    return r;
}

char* amiga_hunks::write32be(char* ptr, uint32_t r)
{
    *ptr++ = r >> 24;
    *ptr++ = r >> 16;
    *ptr++ = r >> 8;
    *ptr++ = r;
    return ptr;
}

char* amiga_hunks::write16be(char* ptr, uint16_t r)
{
    *ptr++ = r >> 8;
    *ptr++ = r;
    return ptr;
}

/**
 *
 *
 * @return Lower 16 bits: 0 if error, otherwise number of found hunks
 *         Upper 16 bits: additional information like errors.
 */

uint32_t amiga_hunks::parse_hunks(const char* ptr, int size, std::vector<hunk_info_t>& hunk_list, bool verbose)
{
    const char* end = ptr+size;
    uint32_t n, m, j;
    uint32_t tsize, tnum, tmax;         // Follow DOS RKRM ;)
    uint32_t msj, mtj;
    uint32_t num_residents;              // archaic stuff
    uint32_t hunk_type;
    int hunk_size;
    int padding;
    int reloc_size;
    bool overlay = false;

    hunk_type = read32be(ptr);
    
    if (verbose) {
        std::cout << ">- Executable file Hunk Parser -------------------------" << std::endl;
    }
    if (size % 4) {
        std::cerr << "**Error: Amiga executable size must be 4 byte aligned." << std::endl;
        return 0;
    }
    if (size < 6*4 || (hunk_type != HUNK_HEADER)) { 
        std::cerr << "**Error: not Amiga executable, no HUNK_HEADER found." << std::endl;
        return 0;
    }
    if (verbose) {
        std::cout << "HUNK_HEADER (0x" << std::hex << hunk_type << ")" << std::endl;
    }

    num_residents = 0;
    while ((n = read32be(ptr))) {
        ptr += (n * 4);
        ++num_residents;
    }

    if (num_residents > 0 && verbose) {
        std::cout << "  File has " << num_residents << " resident libraries.." << std::endl;
    }

    tsize = read32be(ptr);
    tnum  = read32be(ptr);
    tmax  = read32be(ptr);
   
    if (verbose) {
        std::cout << "  Number of segments: " << tsize << std::endl;
        std::cout << "  First segment:      " << tnum << std::endl;
        std::cout << "  Last of segment:    " << tmax << std::endl;
    }
    if (num_residents != tnum) {
        std::cerr << "  First root hunk (" << tnum << ") does not match the number of residents" << std::endl;
        return 0;
    }
    if ((tsize - 1 > tmax) && verbose) {
        std::cout << "  Likely an OVERLAY file" << std::endl;
    }

    hunk_list.reserve(tsize);

    for (n = tnum, m = 0; n <= tmax; n++, m++) {
        hunk_info_t hunk;
        hunk.hunks_remaining = tmax-tnum-m;
        hunk.old_segment_num = n;
        hunk.new_segment_num = -1;
        hunk.reloc_size = 0;
        hunk.short_reloc = false;
        hunk.segment_start = NULL;
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
        if (verbose) {
            std::cout << std::dec << "  Segment: " << n << ", memory size: " << msj << " bytes "
                << "(" << s_memtype_str[mtj] << ")" << std::endl;
        }

        hunk.memory_size = msj;
        hunk.memory_type = mtj;
        hunk_list.push_back(hunk);
    }

    hunk_size = 0;
    n = -1;

    while (ptr < end) {
        hunk_size = 0;
        msj = read32be(ptr);
        // Bits 31 and 30 are unused
        // Bit 29 contains flag for possible hunk advisory..
        hunk_type = msj & 0x1fffffff;
        
        if (verbose && hunk_type <= LAST_SUPPORTED_HUNK) {
            std::cout << s_hunk_str[hunk_type-FIRST_SUPPORTED_HUNK] << "(0x" << std::hex 
                << hunk_type << ")" << std::endl;
        } 
        // skip memory type but parse advisory hunk flag
        if (msj & 0x20000000) {
            hunk_size = read32be(ptr);

            if (verbose) {
                std::cout << "  Advisory hunk flag seen.. skipping " << hunk_size
                    << " long words in hunk " << tnum << std::endl;
                ptr += hunk_size;
            }
        }

        switch (hunk_type) {
        case HUNK_SYMBOL:
            if (verbose) {
                std::cout << "  Removing.." << std::endl;
            }
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
            if (verbose) {
                std::cout << "  Removing.." << std::endl;
            }
            hunk_size = read32be(ptr);
            break;
        case HUNK_DATA:
        case HUNK_CODE:
            hunk_size = read32be(ptr);
        case HUNK_BSS:
            // we increment to next segment due exe files that may not
            // contain HUNK_ENDs
            ++n;
            hunk_list[n].segment_start = ptr;
            hunk_list[n].data_size = hunk_size << 2;
            hunk_list[n].hunk_type = hunk_type;
            hunk_list[n].combined_type |= hunk_type;
            if (verbose) {
                std::cout << std::dec << "  segment: " << hunk_list[n].old_segment_num 
                    << ", data size: " << hunk_list[n].data_size
                    << ", memory size: " << hunk_list[n].memory_size 
                    << ", remaining: " << hunk_list[n].hunks_remaining << std::endl;
            }
            break;
        case HUNK_END:
            hunk_size = 0;
            break;
        case HUNK_RELOC32:
        case HUNK_RELRELOC32:
            hunk_list[n].reloc_start = ptr;
            hunk_list[n].short_reloc = false;
            hunk_size = 0;
            reloc_size = 0;
            do {
                m = read32be(ptr);
                if (m == 0) {
                    break;
                }
                // Skip "to which hunk relocation is to"
                j = read32be(ptr);
                if (verbose) {
                    std::cout << std::dec << "  " << m << " relocs to segment " << j << std::endl;
                }
                // Skip reloc entries
                ptr += (m * 4);
                // num relocs, to hunk and relocs
                reloc_size = reloc_size + 8 + m * 4;
            } while (true);

            // end mark
            hunk_list[n].reloc_size = reloc_size + 4;
            break;
        case HUNK_DREL32:
        case HUNK_RELOC32SHORT:
            padding = 1;
            hunk_list[n].reloc_start = ptr;
            hunk_list[n].short_reloc = true;
            hunk_size = 0;
            reloc_size = 0;
            
            do {
                m = read16be(ptr);
                if (m == 0) {
                    break;
                }
                // Skip "to which hunk relocation is to"
                j = read16be(ptr);
                if (verbose) {
                    std::cout << std::dec << "  " << m << " relocs to segment " << j << std::endl;
                }
                // Skip reloc entries
                ptr += (m * 2);
                padding += (m * 2);
                // 16 bit relocs are internally hansled as 32 bit anyway..
                reloc_size = reloc_size + 8 + m * 8;
            } while (true);
            
            if (padding & 1) {
                ++ptr;
                reloc_size += 2;
            }

            hunk_list[n].reloc_size = reloc_size + 8;
            break;
        case HUNK_BREAK:
            break;
        case HUNK_OVERLAY:
            overlay = true;
            // Definitely not sure if this is enough to parse overlay files..
            hunk_size = read32be(ptr) + 1;
            break;
        default:
            std::cerr << "**Error: unsupported hunk 0x" << std::hex << hunk_type << std::endl;
            free_hunk_info(hunk_list);
            return 0;
        }

        ptr += (hunk_size * 4);
    }

    if (++n < tsize && verbose) {
        std::cout << "Number of parsed segments does not match with the header information.." << std::endl;
    }
    if (overlay) {
        return n | MASK_OVERLAY_EXE;
    } else {
        return n;
    }
}


void amiga_hunks::free_hunk_info(std::vector<hunk_info_t>& hunk_list)
{
    hunk_list.resize(0);
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
int amiga_hunks::optimize_hunks(std::vector<hunk_info_t>& hunk_list, char*& new_exe, int len) {
    (void)len;

    int o, m, n;
    int number_old_segments = hunk_list.size();
    //int* segment_mapping = new int[number_old_segments];
    uint32_t* merged_hunk_type;
    uint32_t* merged_data_size;
    uint32_t* merged_mem_size;
    std::map<uint32_t,int> new_hunks;
    int number_new_segments = 0;
    new_exe = new char[len];
    char* ptr = new_exe;
    int total_reloc_size = 0;
    char* p_reloc, *t_reloc;

    int* old_seg_size = new int[number_old_segments];
    int* old_seg_bss = new int[number_old_segments];
    int* old_seg_offs = new int[number_old_segments];
    int* old_seg_bss_offs = new int[number_old_segments];

    // Find mergeable hunks/segments

    //for (std::move_iterator<std::vector<hunk_info_t>::iterator> it{hunk_list.begin()}, et{hunk_list.end()}; it != et; it++) {
    for (std::vector<hunk_info_t>::iterator  it = hunk_list.begin(), et = hunk_list.end(); it != et; it++) {
        if (new_hunks.find(it->combined_type) == new_hunks.end()) {
            new_hunks[it->combined_type] = number_new_segments++;
        }
        it->new_segment_num = new_hunks[it->combined_type];
        old_seg_size[it->old_segment_num] = it->data_size;
        old_seg_bss[it->old_segment_num] = it->memory_size - it->data_size;
    }

    merged_data_size = new uint32_t[number_new_segments];
    merged_hunk_type = new uint32_t[number_new_segments];
    merged_mem_size  = new uint32_t[number_new_segments];

    for (m = 0; m < number_new_segments; m++) {
        merged_data_size[m] = 0;
        merged_mem_size[m] = 0;
        old_seg_offs[m] = 0;
    }
    for (m = 0; m < number_old_segments; m++) {
        total_reloc_size += hunk_list[m].reloc_size;
        old_seg_offs[m] = merged_data_size[new_hunks[hunk_list[m].combined_type]]; 
    
        merged_hunk_type[hunk_list[m].new_segment_num] = hunk_list[m].combined_type;
        merged_data_size[hunk_list[m].new_segment_num] += hunk_list[m].data_size;
        merged_mem_size[hunk_list[m].new_segment_num] += hunk_list[m].memory_size;
        
        old_seg_size[m] = hunk_list[m].data_size;
    }
    for (m = 0; m < number_new_segments; m++) {
        std::cout << "Merged new segment " << m << ":" << std::endl;
        std::cout << "  Hunk type -> " << merged_hunk_type[m] << std::endl;
        std::cout << "  Sizes  -> " << merged_data_size[m] << std::endl;
    }
    for (m = 0; m < number_old_segments; m++) {
        std::cout << "Old segment " << m << ":" << std::endl;
        std::cout << "  Mapped to -> " << hunk_list[m].new_segment_num << std::endl;
        std::cout << "  Offset -> " << old_seg_offs[m] << std::endl;  
        std::cout << "  Size -> " << old_seg_size[m] << std::endl;  
        std::cout << "  BSS -> " << old_seg_bss[m] << std::endl;
    }

    // Merge datas..
    // Output the "HUNK_HEADER"

    ptr = write16be(ptr,number_new_segments);
    for (m = 0; m < number_new_segments; m++) {
        ptr = write32be(ptr,(merged_mem_size[m] &0xfffffffc) | (merged_hunk_type[m] >> 30));
    }

    // Output each new segment
    for (m = 0; m < number_new_segments; m++) {
        bool move_data = true;
        // output segment memory type and data size in 32bit long words
        switch (merged_hunk_type[m] & 0x3fffffff) {
            case HUNK_CODE:
                ptr = write32be(ptr,merged_data_size[m] | SEGMENT_TYPE_CODE);
                break;
            case HUNK_DATA:
                ptr = write32be(ptr,merged_data_size[m] | SEGMENT_TYPE_DATA);
                break;
            case HUNK_BSS:
                ptr = write16be(ptr,SEGMENT_TYPE_BSS);
                move_data = false;
                break;
            default:
                assert("Unknown hunk type" == NULL);
                break;
        }

        if (move_data) {
            // Not the greatest solution as we do unnecessary loops over the 
            // old list of segments..
            for (n = 0; n < number_old_segments; n++) {
                if (m == hunk_list[n].new_segment_num) {
                    o = hunk_list[n].data_size;

                    while (o-- > 0) {
                        *ptr++ = hunk_list[m].segment_start[o];
                    }
                }
            }
        }
    }


    n = (ptr-new_exe);
    std::cout << ">> New exe size before reloc data: " << n << std::endl;

#if 1   // just for the debug
    std::ofstream ofs;
    ofs.open("h.bin",std::ios::binary|std::ios::out);
    ofs.write(new_exe,n);
    ofs.close();
#endif

    // collect relocations
    p_reloc = new char[total_reloc_size];
    t_reloc = p_reloc;


    for (int reloc_to_seg = 0; reloc_to_seg < number_new_segments; reloc_to_seg++) {
        int reloc_base_addr = 0;

        // relocate this segment
        t_reloc = write32be(t_reloc,reloc_to_seg);

        for (int old_seg_num = 0; old_seg_num < number_old_segments; old_seg_num++) {
            int new_seg_num = hunk_list[old_seg_num].new_segment_num;

            if (new_seg_num == reloc_to_seg && hunk_list[old_seg_num].reloc_start) {
                // Collect relocs and adjust the base address 



                t_reloc = write32be(t_reloc,reloc_to_seg); 

            }
        }
    }


    //

    for (m = 0; m < number_new_segments; m++) {
        std::cout << "  Merged segment end  -> " << m << ": " << merged_data_size[m] << std::endl;
    }


    std::cout << "Number of merged hunks is " << number_new_segments << std::endl;

    // Just for my own education.. Did not remember this ;)
    // https://stackoverflow.com/questions/7648756/is-the-order-of-iterating-through-stdmap-known-and-guaranteed-by-the-standard
    for (auto& i: new_hunks)  {
        std::cout << "Combined mapping -> " << i.first << ": " << i.second << std::endl;
    }

    m = 0;

    delete[] p_reloc;
    delete[] merged_data_size;
    delete[] merged_hunk_type;
    delete[] merged_mem_size;
    delete[] old_seg_size;
    delete[] old_seg_offs;
    delete[] old_seg_bss;
    delete[] old_seg_bss_offs;
    return 0;
}



