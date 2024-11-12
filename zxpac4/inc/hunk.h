/**
 * @file hunk.h
 * @brief Amiga executable hunk parsing and processing.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 *
 * @copyright The Unlicense
 *
 */

#ifndef HUNK_H_INCLUDED_
#define HUNK_H_INCLUDED_

#include <iostream>
#include <iomanip>
#include <vector>
#include <cassert>
#include <stdint.h>



#define HUNK_UNIT	        0x3E7       // error
#define FIRST_SUPPORTED_HUNK    HUNK_UNIT
#define HUNK_NAME	        0x3E8       // exe remove
#define HUNK_CODE	        0x3E9       // exe -> 00
#define HUNK_DATA	        0x3EA       // exe -> 01
#define HUNK_BSS	        0x3EB       // exe -> 10
#define HUNK_RELOC32	    0x3EC       // exe -> 11
#define HUNK_RELOC16	    0x3ED       // error
#define HUNK_RELOC8	        0x3EE       // error
#define HUNK_EXT		    0x3EF       // error
#define HUNK_SYMBOL		    0x3F0       // exe remove
#define HUNK_DEBUG		    0x3F1       // exe remove
#define HUNK_END		    0x3F2       // exe
#define HUNK_HEADER		    0x3F3       // exe
#define HUNK_OVERLAY	    0x3F5       // output exe / error
#define HUNK_BREAK		    0x3F6       // output exe / error
#define HUNK_DREL32		    0x3F7       // error
#define HUNK_DREL16		    0x3F8       // error
#define HUNK_DREL8		    0x3F9       // error
#define HUNK_LIB		    0x3FA       // error
#define HUNK_INDEX		    0x3FB       // error
#define HUNK_RELOC32SHORT	0x3FC       // exe -> 11
#define HUNK_RELOC32SHORT_OLD  0x3f7    // exe -> 11
#define HUNK_RELRELOC32	    0x3FD       // exe
#define HUNK_ABSRELOC16	    0x3FE       // error
#define LAST_SUPPORTED_HUNK HUNK_ABSRELOC16
#define HUNK_PPC_CODE7	    0x4E9       // error
#define HUNK_RELRELOC26     0x4EC       // error

//

#define MASK_OVERLAY_EXE        0x80000000
#define MASK_UNSUPPORTED_HUNK   0x40000000

//
#define MEMF_ANY        0
#define MEMF_PUBLIC     1
#define MEMF_CHIP       2
#define MEMF_FAST       4



/*
 
 Hunk merger/optimizer 
  - merge hunks whenever possible into larger one
  - sort relocs
  - apply reloc32short when possible
  - remove NAME/DEBUG/SYMBOL hunks
  - do not support Advisory hunk flag -> error




 */



namespace amiga_hunks {
    typedef struct {
        int hunks_remaining;
        uint32_t hunk_type;
        uint32_t hunk_num;
        uint32_t memory_size;
        uint32_t data_size;
        uint32_t memory_type;
        uint32_t* segment_start;    //< 
        uint32_t* reloc_start;
        bool short_reloc:1;
    } hunk_info_t;

    uint32_t read32be(const uint32_t* ptr);
    uint16_t read16be(const uint16_t* ptr);
    uint32_t parse_hunks(const char* buf, int size, std::vector<hunk_info_t>& hunk_list, bool verbose=false);
    void free_hunk_info(std::vector<hunk_info_t>& hunk_list);
    int optimize_hunks(std::vector<hunk_info_t>& hunk_list, char*& new_exe, int len);
};





#endif  //  HUNK_H_INCLUDED_
