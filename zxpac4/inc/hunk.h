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
#include <map>
#include <set>

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
        int old_seg_num;
        int new_seg_num;
        int memory_size;
        int data_size;
        int merged_start_index;
        uint32_t memory_type;
        uint32_t combined_type;     // hunk_type | (memory_type << 30)
        char* seg_start;    //
        char* reloc_start;
        std::map<int,std::set<uint32_t> > relocs;    ///< Key is dst_segment, value is reloc
    } hunk_info_t;

    uint32_t read32be(char*& ptr, bool inc=true);
    uint16_t read16be(char*& ptr, bool inc=true);
    uint32_t readbe(char*& ptr, int bytes, bool inc=true);
    char* write32be(char* ptr, uint32_t v, bool inc=true);
    char* write24be(char* ptr, uint32_t r, bool inc=true);
    char* write16be(char* ptr, uint16_t v, bool inc=true);
    int parse_hunks(char* buf, int size, std::vector<hunk_info_t>& hunk_list, bool debug=false);
    int merge_hunks(char* exe, int len, std::vector<hunk_info_t>& hunk_list, char*& new_exe,
        std::vector<uint32_t>* new_segments, bool debug=false);
    int optimize_hunks(char* exe, int len, const std::vector<hunk_info_t>& hunk_list, char*& new_exe,
        std::vector<uint32_t>* new_segments, bool debug=false);
};

#define TDEBUG(x) {if (debug) {x}}

#define MEMORY_TYPE_MEMF_ANY    0x00
#define MEMORY_TYPE_MEMF_CHIP   0x01
#define MEMORY_TYPE_MEMF_FAST   0x02
#define MEMORY_TYPE_MEMF_RESV   0x03

#define SEGMENT_TYPE_RELOC      0x0000
#define SEGMENT_TYPE_CODE       0xc0000000
#define SEGMENT_TYPE_DATA       0x80000000
#define SEGMENT_TYPE_BSS        0x4000
#define SEGMENT_TYPE_MASK       0xc000
#define SEGMENT_TYPE_EOF        0x0000
#define MAX_SEGMENT_NUM         0x3ffe

#define MAX_RELOC_OFFSET        0x00ffffff

/*
 *
  Hunk types:
    11nnnnnnnnnnnnnn + nnnnnnnnnnnnnnnn  -> HUNK_CODE (data size nnnnnnnnnnnnnnnnnnnnnnnnnnnnnn << 2)
    10nnnnnnnnnnnnnn + nnnnnnnnnnnnnnnn  -> HUNK_DATA
    0100000000000000                     -> HUNK_BSS (size is implicitly known)
    0000000000000000                     -> EOF
    00nnnnnnnnnnnnnn                     -> RELOCS (nnnnnnnnnnnnnn > 0)

  All segments' data is together followed but all relocation data. Unlinke in normal executable
  layout the relocation information is not after each segment.

  Relocation information:
    00dddddddddddddd + 00ssssssssssssss  -> reloc within "ss...s" segment+1 to "dd..d" segment+1
                                         -> if "sss.s" is 0x0000 then EOF
    rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr      -> First 32bit reloc to which deltas are applied to.
                                            It could be 24bits but 32 is easier to decompressor.
  Reloc entry
    0rrrrrrr                             -> 7 lowest bits of reloc delta
    1rrrrrrr                             -> 7 upper bits of reloc delta and read next byte for next 7bits
    00000000 + [00000000]                -> End of relocs + optional byte to align delta relocs to 16bit
    Note1: reloc entries are always even thus the delta is shift to right before encoding.
    Note2: reloc delta entry cannot be 0, thus it can be used as an end marker.
    
    examples: 
      0aaaaaaa                           -> 8bit  reloc delta aaaaaaa0
      1aaaaaaa + 0bbbbbbb                -> 15bit reloc delta aaaaaaabbbbbbb0
      1aaaaaaa + 1bbbbbbb + 0ccccccc     -> 22bit reloc delta aaaaaaabbbbbbbccccccc0
      00000000                           -> end of relocs for this hunk.


 */



#endif  //  HUNK_H_INCLUDED_
