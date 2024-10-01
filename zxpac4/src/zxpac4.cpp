/**
 * @file zxpac4.cpp
 * @brief zxpac4 for ASCII only data and binary data
 * @version 0.1
 * @author Jouni Korhonen
 * @date somewhere in 2024
 *
 * @copyright The Unlicense
 *
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <new>
#include <cstring>
#include "zxpac4.h"
#include "lz_util.h"

#include <cctype>
#include <cassert>


zxpac4::zxpac4(const lz_config* p_cfg, int ins, int max) :
    lz_base(p_cfg),
    m_lz(NULL),
    m_cost_array(NULL),
    //m_mtf(256,129,ins,1,max),
    m_cost(p_cfg)
{
    (void)ins;
    (void)max;

    try {
        m_lz = new hash3(p_cfg->window_size,
            p_cfg->min_match,
            p_cfg->max_match,
            p_cfg->good_match,
            p_cfg->min_match2_threshold,
            p_cfg->min_match3_threshold);  // may throw exception
    }
    catch (std::exception &e) {
        m_lz = NULL;
        throw e;
    }

    m_alloc_len  = 0;
}

zxpac4::~zxpac4(void)
{
    if (m_lz) {
        delete m_lz;
    }
    if (m_cost_array) {
        lz_cost_array_done();
    }
}



int zxpac4::impl_search_matches(const char* buf, int len, int interval)
{
    int pos = 0;
    int num;

    // Unused at the moment..
    (void)interval;

    // init statistics
    m_num_literals = 0;
    m_num_pmr_literals = 0;
    m_num_matches = 0;
    m_num_matched_bytes = 0;
    m_num_pmr_matches = 0;

    if (m_verbose) {
        std::cout << "Finding all matches" << std::endl;
    }

    // find matches..
    if (m_debug && m_verbose) {
        std::cerr << ">- Match debugging phase --------------------------------------------------------\n";
        std::cerr << "  file pos: asc (hx) -> offset:length(s) or 'no macth'\n";
    }

    while (pos < len) {
        m_lz->init_get_matches(m_lz_config->max_chain,m_cost_array[pos].matches);
            
        // Find all matches at this position. Returned 'num' is the
        // the number of found matches.
        num = m_lz->find_matches(buf,pos,len-pos,m_lz_config->only_better_matches);
        m_cost_array[pos].num_matches = num;

        if (m_debug && m_verbose) {
            std::cerr << std::setw(10) << std::dec << std::setfill(' ') << pos << ": '"
                << (std::isprint(buf[pos]) ? buf[pos] : ' ')
                << "' (" << std::setw(2) << std::setfill('0') << std::hex << (buf[pos] & 0xff)
                << std::dec << std::setw(0) <<  ") -> " << num << ": ";
            if (num > 0) {
                for (int n = 0; n < num; n++) {
                    assert(m_cost_array[pos].matches[n].offset < 131072);
                    std::cerr << m_cost_array[pos].matches[n].offset << ":"
                              << m_cost_array[pos].matches[n].length << " ";
                }
            } else {
                std::cerr << "no match";
            }
            std::cerr << "\n";
        }
        ++pos;
    }

    return 0;
}


int zxpac4::impl_parse(const char* buf, int len, int interval)
{
    int length;
    int offset;
    int pos;
    int num_literals;
    int pmr_offset = m_lz_config->initial_pmr_offset;

    // Unused at the moment..
    (void)interval;

    if (m_verbose) {
        if (m_lz_config->reversed_file) {
            std::cout << "Reversed file parsing engaged" << std::endl;
        } else {
            std::cout << "History parsing engaged" << std::endl;
        }
    }

    if (m_verbose) {
        std::cout << "Calculating arrival costs" << std::endl;
    }
    
    m_cost.init_cost(m_cost_array,0,len,pmr_offset);

    for (pos = 0; pos < len; pos++) {
        // always do literal cost calculation
        m_cost.impl_literal_cost(pos,m_cost_array,buf);
        
        // match cost calculation if not at the end of file and there was a match
        if (pos < (len - MATCH_MIN)) {
            m_cost.impl_match_cost(pos,m_cost_array,buf);
        }        
    }

    //
    if (m_verbose) {
        std::cout << "Building list of optimally parsed matches" << std::endl;
    }
    
    pos = len;
    num_literals = 1;

    // Fix the links of selected cost nodes
    // Also substitute PMRs for the forward parser
    while (pos > 0) {
        length = m_cost_array[pos].length;
        offset = m_cost_array[pos].offset;
        assert(length > 0);
        m_cost_array[pos].num_literals = 0;

        if (length == 1 && offset == 0) {
            m_cost_array[pos].num_literals = num_literals++;
        } else {
            // reset literal..
            num_literals = 1;
        }

        // Collect statistics..
        if (offset == 0 && length > 1) {
            ++m_num_pmr_matches;
            ++m_num_matches;
            m_num_matched_bytes += length;
        } else if (offset > 0 && length == 1) {
            ++m_num_pmr_literals;
            ++m_num_literals;
        } else if (offset == 0 && length == 1) {
            ++m_num_literals;
        } else {
            ++m_num_matches;
            m_num_matched_bytes += length;
        }
        if (pos == len) {
            m_cost_array[pos].next = 0;
        } 
        m_cost_array[pos-length].next = pos;
        pos -= length;
    }

    assert(m_cost_array[1].num_literals >= 1);

    if (m_debug) {
        std::cerr << ">- Cost debugging phase ------------------------------------------------------" << std::endl;
        std::cerr << "  file pos: asc (hx) #lit (pmroff) offset:len  arri_cost ->     nxtpos pmr" << std::endl;
        pos = 0;

        while (pos < len+1) {
            std::cerr 
                // cost array position
                << std::setw(10) << std::setfill(' ') << pos << ": '"
                // ascii character at the same file position
                << (std::isprint(buf[pos-1]) ? buf[pos-1] : ' ') << "' ("
                // same as hexadecimal
                << std::hex << std::setfill('0') << std::setw(2) << (buf[pos-1] & 0xff) << ") "
                // length of the literal run
                << std::dec << std::setw(4) << std::setfill(' ') << m_cost_array[pos].num_literals 
                // PMR offset
                << " (" << std::setw(6) << m_cost_array[pos].pmr_offset << ") "
                // match offset, may be 0 if PMR was selected
                << std::setw(6) << m_cost_array[pos].offset
                // match length
                << ":" << std::setw(3) << m_cost_array[pos].length 
                // arrival cost
                << " " << std::setw(10) << m_cost_array[pos].arrival_cost
                // next position in cost array
                << " -> " << std::setw(10) << m_cost_array[pos].next << " " 
                // mark possible PMR match
                << (m_cost_array[pos].offset > 0 && m_cost_array[pos].length == 1 ? "(l)" : "")
                << (m_cost_array[pos].offset == 0 && m_cost_array[pos].length > 1 ? "(m)" : "")
                << "\n";
            ++pos;
        }
    
        std::cerr << ">- Show final selected -------------------------------------------------------" << std::endl;
        std::cerr << "     file pos: asc (hx) #lit (pmroff) offset:len  arri_cost ->     nxtpos pmr    " << std::endl;
        pos = m_cost_array[0].next;

        do {
            std::cerr 
                // Final debug marker
                << "F: "
                // cost array position
                << std::setw(10) << std::setfill(' ') << pos << ": '"
                // ascii character at the same file position
                << (std::isprint(buf[pos-1]) ? buf[pos-1] : ' ') << "' ("
                // same as hexadecimal
                << std::hex << std::setfill('0') << std::setw(2) << (buf[pos-1] & 0xff) << ") "
                // length of the literal run
                << std::dec << std::setw(4) << std::setfill(' ') << m_cost_array[pos].num_literals 
                // PMR offset
                << " (" << std::setw(6) << m_cost_array[pos].pmr_offset << ") "
                // match offset, may be 0 if PMR was selected
                << std::setw(6) << m_cost_array[pos].offset
                // match length
                << ":" << std::setw(3) << m_cost_array[pos].length 
                // arrival cost
                << " " << std::setw(10) << m_cost_array[pos].arrival_cost
                // next position in cost array
                << " -> " << std::setw(10) << m_cost_array[pos].next << " " 
                // mark possible PMR match
                << (m_cost_array[pos].offset > 0 && m_cost_array[pos].length == 1 ? "(L)" : "")
                << (m_cost_array[pos].offset == 0 && m_cost_array[pos].length > 1 ? "(M)" : "")
                << "\n";
            pos = m_cost_array[pos].next;
        } while (pos > 0);
    }
    return 0;
}


const cost* zxpac4::impl_cost_array_get(int len)
{
    if (len < 1) {
       return NULL;
    }
    if (len < m_alloc_len && m_cost_array) {
        return m_cost_array;
    } else {
        impl_cost_array_done();
    }

    m_cost_array = static_cast<cost_with_ctx*>(m_cost.alloc_cost(len,m_lz_config->max_chain)); 
    m_alloc_len = len;

    return m_cost_array;
}

void zxpac4::impl_cost_array_done(void)
{
    m_cost.free_cost(m_cost_array); 
    m_alloc_len = 0;
    m_cost_array = NULL;
}

int zxpac4::encode_history(putbits* pb, const char* buf, char* p_out, int len, int pos)
{
    char* last_literal_ptr;
    char literal;
    int tag;
    int length;
    int offset;
    int n;

    // Build header at the beginning of the file.. max 16M files supported.
    if (m_lz_config->is_ascii) {
        pb->byte(static_cast<char>(0x80 | m_lz_config->initial_pmr_offset));
    } else {
        pb->byte(m_lz_config->initial_pmr_offset);
    }
    pb->byte(len >> 16);
    pb->byte(len >> 8);
    pb->byte(len >> 0);
    
    // Always send first literal (which cannot be compressed without a tag..
    pos = m_cost_array[0].next;
    literal = buf[0];

    // store compressed file..
    if (m_lz_config->is_ascii) {
        last_literal_ptr = pb->byte(literal<<1);
        if (m_debug && m_verbose) {
            std::cerr << "O: Initial literal -> last_ptr = " 
                << last_literal_ptr - p_out << " -> " << literal << "\n";
        }
    } else {
        pb->byte(literal);
        last_literal_ptr = NULL;
        
        if (m_debug && m_verbose) {
            std::cerr << "O: Initial literal -> last_ptr = NULL -> " 
                << literal << "\n";
        }
    }
    
    while ((pos = m_cost_array[pos].next)) {
        length = m_cost_array[pos].length;
        offset = m_cost_array[pos].offset;
        literal = buf[pos-1];

        if (offset == 0 && length == 1) {
            // encode raw literal
            if (m_debug && m_verbose) {
                std::cerr << "O: Literal ";
            }
            if (m_lz_config->is_ascii && last_literal_ptr) {
                // Unnecessary..
                if (m_debug && m_verbose) {
                    std::cerr << "last_ptr = " << std::dec << last_literal_ptr-p_out;
                }
                *last_literal_ptr &= 0xfe;
            } else {
                pb->bits(0,1);

                if (m_debug && m_verbose) {
                    std::cerr << "bits(0,1)";
                }
            }
            if (m_lz_config->is_ascii) {
                last_literal_ptr = pb->byte(literal<<1);
            } else {
                pb->byte(literal);
            }
            if (m_debug && m_verbose) {
                std::cerr << " -> " << std::hex << literal << "\n";
            }
        } else {
            if (m_debug && m_verbose) {
                if (m_debug) {
                    std::cerr << "O: Match ";
                }
            }
            if (m_lz_config->is_ascii && last_literal_ptr) {
                if (m_debug && m_verbose) {
                    std::cerr << "last_ptr = " << std::dec << last_literal_ptr-p_out;
                }
                
                *last_literal_ptr |= 0x01;
            } else {
                if (m_debug && m_verbose) {
                    std::cerr << "bits(1,1)";
                }

                pb->bits(1,1);
            }
            if ((offset == 0 && length > 1) || (offset > 0 && length == 1)) {
                // encode match PMR or literal PMR
                if (m_debug && m_verbose) {
                    std::cerr << ", PMR bits(1,1) Length " << length;
                }

                pb->bits(1,1);
                n = m_cost.get_length_tag(length,tag);
                pb->bits(tag,n);
            
                if (m_debug && m_verbose) {
                    std::cerr << ", bits " << n << "\n";
                }
            } else {
                // encode match
                if (m_debug && m_verbose) {
                    std::cerr << std::dec << ", Offset " << offset << ", Length " << length;
                }
                
                pb->bits(0,1);
                int m = m_cost.get_offset_tag(offset,literal,tag);
                pb->byte(literal);

                if (m > 0) {
                    pb->bits(tag,m);
                }
                n = m_cost.get_length_tag(length-1,tag);
                pb->bits(tag,n);
                
                if (m_debug && m_verbose) {
                    std::cerr << ", bits " << m+n+8+1 << "\n";
                }
            }
            last_literal_ptr = NULL;
        }
    }

    n = pb->flush() - p_out;
    return n;
}


int zxpac4::encode_forward(putbits* pb, const char* buf, char* p_out, int len, int pos)
{
    char* last_literal_ptr;
    char literal;
    int tag;
    int length;
    int offset;
    int n;

    assert(false);

    pos = m_cost_array[0].next;
    
    while ((pos = m_cost_array[pos].next)) {
        length = m_cost_array[pos].length;
        offset = m_cost_array[pos].offset;
        literal = buf[pos-1];

        if (offset == 0 && length == 1) {
            // encode raw literal
            if (m_debug && m_verbose) {
                std::cerr << "O: Literal ";
            }
            if (m_lz_config->is_ascii && last_literal_ptr) {
                // Unnecessary..
                if (m_debug && m_verbose) {
                    std::cerr << "last_ptr = " << std::dec << last_literal_ptr-p_out;
                }
                ////*last_literal_ptr &= 0xfe;
            } else {
                pb->bits(0,1);

                if (m_debug && m_verbose) {
                    std::cerr << "bits(0,1)";
                }
            }
            if (m_lz_config->is_ascii) {
                last_literal_ptr = pb->byte(literal<<1);
            } else {
                pb->byte(literal);
            }
            if (m_debug && m_verbose) {
                std::cerr << " -> " << std::hex << literal << "\n";
            }
        } else {
            if (m_debug && m_verbose) {
                if (m_debug) {
                    std::cerr << "O: Match ";
                }
            }
            if (m_lz_config->is_ascii && last_literal_ptr) {
                if (m_debug && m_verbose) {
                    std::cerr << "last_ptr = " << std::dec << last_literal_ptr-p_out;
                }

                ///*last_literal_ptr |= 0x01;
            } else {
                if (m_debug && m_verbose) {
                    std::cerr << "bits(1,1)";
                }

                pb->bits(1,1);
            }
            if ((offset == 0 && length > 1) || (offset > 0 && length == 1)) {
                // encode match or literal PMR
                if (m_debug && m_verbose) {
                    std::cerr << ", PMR bits(1,1) Length " << length;
                }

                pb->bits(1,1);
                n = m_cost.get_length_tag(length,tag);
                pb->bits(tag,n);
            
                if (m_debug && m_verbose) {
                    std::cerr << ", bits " << n << "\n";
                }
            } else {
                // encode match
                if (m_debug && m_verbose) {
                    std::cerr << std::dec << ", Offset " << offset << ", Length " << length;
                }
                
                pb->bits(0,1);
                int m = m_cost.get_offset_tag(offset,literal,tag);
                pb->byte(literal);

                if (m > 0) {
                    pb->bits(tag,m);
                }
                n = m_cost.get_length_tag(length-1,tag);
                pb->bits(tag,n);
                
                if (m_debug && m_verbose) {
                    std::cerr << ", bits " << m+n+8+1 << "\n";
                }
            }
            last_literal_ptr = NULL;
        }
    }

    // Always send first literal (which cannot be compressed without a tag..
    pos = m_cost_array[0].next;
    literal = buf[0];

    // store compressed file..
    if (m_lz_config->is_ascii) {
        last_literal_ptr = pb->byte(literal<<1);
    } else {
        pb->byte(literal);
        last_literal_ptr = NULL;
    }


    // Build header at the end of the file.. max 16M files supported.
    if (m_lz_config->is_ascii) {
        pb->byte(static_cast<char>(0x80));
    } else {
        pb->byte(0x00);
    }
    pb->byte(len >> 16);
    pb->byte(len >> 8);
    pb->byte(len >> 0);
    n = pb->flush() - p_out;
    return n;
}



int zxpac4::impl_encode(const char* buf, int len, std::ofstream& ofs)
{
    int n;
    char* p_out;
    putbits* pb;

    n = m_cost_array[len].arrival_cost;
    n = n + m_num_literals;

    assert(n < LZ_MAX_COST);
    n = (n + 7) / 8;
    
    if (n >= len) {
        return -1;
    }
    if (( p_out = new(std::nothrow) char[n+ZXPAC4_HEADER_SIZE]) == NULL) {
        return -2;
    }
    
    pb = new putbits_history(p_out);
    
    if (m_lz_config->reversed_file) {
        if (m_verbose) {
            std::cout << "Either reversed file or forward encoding engaged" << std::endl;
        }
        n = encode_forward(pb,buf,p_out,len,0) + 4;
    } else {
        if (m_verbose) {
            std::cout << "History encoding engaged" << std::endl;
        }
        n = encode_history(pb,buf,p_out,len,0);
    }
    if (m_verbose) {
        std::cout << "Compressed length: " << n << std::endl;
    }

    ofs.write(p_out,n);
    delete pb;
    delete[] p_out;

    return n;
}

