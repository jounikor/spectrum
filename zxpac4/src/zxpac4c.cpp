/**
 * @file zxpac4c.cpp
 * @brief zxpac4c for ASCII only data and binary data
 * @version 0.1
 * @author Jouni Korhonen
 * @date somewhere in 2025
 *
 * @copyright The Unlicense
 *
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <new>
#include <cstring>
#include "zxpac4c.h"
#include "lz_util.h"
#include "rice_encoder.h"

#include <cctype>
#include <cassert>
#include <cmath>


zxpac4c::zxpac4c(const lz_config* p_cfg, int ins, int max) :
    lz_base(p_cfg),
    m_lz(p_cfg->window_size,
        p_cfg->min_match,
        p_cfg->max_match,
        p_cfg->good_match,
        p_cfg->min_match2_threshold,
        p_cfg->min_match3_threshold),  // may throw exception
    m_cost_array(NULL),
    m_cost(p_cfg)
{
    (void)ins;
    (void)max;
    m_match_array = new match[p_cfg->max_chain];
    m_alloc_len  = 0;
}

zxpac4c::~zxpac4c(void)
{
    if (m_cost_array) {
        lz_cost_array_done();
    }
}



int zxpac4c::lz_search_matches(char* buf, int len, int interval)
{
    int pos = 0;
    int num;
    int offset, length;

    // Unused at the moment..
    (void)interval;

    // init statistics
    m_num_literals = 0;
    m_num_pmr_literals = 0;
    m_num_matches = 0;
    m_num_matched_bytes = 0;
    m_num_pmr_matches = 0;
    
    m_cost.init_cost(m_cost_array,0,len,m_lz_config->initial_pmr_offset);

    if (m_lz_config->verbose) {
        std::cout << "Finding all matches" << std::endl;
    }

    // find matches..
    if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
        std::cerr << ">- Match debugging phase --------------------------------------------------------\n";
        std::cerr << "  file pos: asc (hx) -> offset:length(s) or 'no macth'\n";
    }

    while (pos < len) {
        m_lz.init_get_matches(m_lz_config->max_chain,m_match_array);


        // Find all matches at this position. Returned 'num' is the
        // the number of found matches.
        num = m_lz.find_matches(buf,pos,len-pos,m_lz_config->only_better_matches);

        if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
            std::cerr << std::setw(10) << std::dec << std::setfill(' ') << pos << ": '"
                << (std::isprint(buf[pos]) ? buf[pos] : ' ')
                << "' (" << std::setw(2) << std::setfill('0') << std::hex << (buf[pos] & 0xff)
                << std::dec << std::setw(0) <<  ") -> " << num << ": ";
            if (num > 0) {
                //std::cerr << num << "\n";
                for (int n = 0; n < num; n++) {
                    assert(m_match_array[n].offset < m_lz_config->window_size);
                    assert(m_match_array[n].length <= m_lz_config->max_match);
                    std::cerr << m_match_array[n].offset << ":"
                              << m_match_array[n].length << " ";
                }
            } else {
                std::cerr << "no match";
            }
            std::cerr << "\n";
        }

        // always do literal cost calculation
        m_cost.literal_cost(pos,m_cost_array,buf);
        
        // match cost calculation if not at the end of file and there was a match
        if (pos < (len - m_lz_config->min_match)) {
            for (int match_pos = 0; match_pos < num; match_pos++) {
                offset = m_match_array[match_pos].offset;
                length = m_match_array[match_pos].length;
                length = m_cost.match_cost(pos,m_cost_array,buf,offset,length);
            }
        }        

        ++pos;
    }

    return 0;
}


int zxpac4c::lz_parse(const char* buf, int len, int interval)
{
    int length;
    int offset;
    int pos;
    int next;
    int num_literals;
    int sym;
    int previous_was_pmr;
	int min_offset_bits = log2(m_lz_config->min_offset);

    // Unused at the moment..
    (void)interval;

    if (m_lz_config->verbose) {
        std::cout << "Calculating arrival costs" << std::endl;
    }
    if (m_lz_config->verbose) {
        std::cout << "Building list of optimally parsed matches" << std::endl;
    }
    
    pos = len;
    num_literals = 1;
    previous_was_pmr = 0;

    // Fix the links of selected cost nodes
    while (pos > 0) {
        length = m_cost_array[pos].length;
        offset = m_cost_array[pos].offset;
        assert(length > 0);
        m_cost_array[pos].num_literals = 0;

        //
        // Following offset and length  combinations exits:
        // 1) offset > 0 && length > 1 -> normal match
        // 2) offset > 0 && length = 1 -> PMR match with length 1 encoded as lireal run
        // 3) offset = 0 && length > 1 -> PMR match encoded as a literal run
        // 4) offset = 0 && length = 1 -> literal
        // 
        if (length == 1 && offset == 0) {
            // Case 4) literal
            m_cost_array[pos].num_literals = num_literals++;
        } else {
            // reset literal..
            num_literals = 1;
        }

        // Collect statistics.. and fix back-to-back PMR cases..
        
        // Case 3) offset = 0 && length > 1 -> PMR match encoded as a literal run
        if (offset == 0 && length > 1) {
            // A normal PMR
			if (previous_was_pmr > 0) {
				// Last PMR match length >= 255.. join with this one previous PMR and skip current
				next = previous_was_pmr;
				m_cost_array[previous_was_pmr].length += length;
			} else {
				previous_was_pmr = pos;
				next = pos;
				++m_num_pmr_matches;
				++m_num_matches;
			}
            m_num_matched_bytes += length;
        // Case 2) offset > 0 && length = 1 -> PMR match with length 1 encoded as lireal run
        } else if (offset > 0 && length == 1) {
            if (previous_was_pmr > 0) {
                // Link this literal PMR to previous PMR match.. There are two cases
                // 1) PMR literal followed by PMR match
                // 2) PMR literal followed by PMR literal
                // The decompressor cannot handle back to back PMRs, thus we must kill
                // one PMR literal and include it into previous PMR match or create a
                // PMR match with length 2.
                if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                    std::cerr << "previous_was_literal " << previous_was_pmr << " and pos " << pos << std::endl;
                }
                if (m_cost_array[previous_was_pmr].length > 1) {
                    if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                        std::cerr << "PMR match" << std::endl;
                    }
                    assert("SHOULD NOT HAPPEN" == NULL);
                } else {
                    if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) { 
                        std::cerr << "PMR literal" << std::endl;
                    }
                    --m_num_pmr_literals;
                    --m_num_literals;
                }
                // Skip this PMR and add in into the previous
                ++m_cost_array[previous_was_pmr].length;
                m_cost_array[previous_was_pmr].offset = 0;
                ++m_num_matched_bytes;
                next = previous_was_pmr;
            } else {
                ++m_num_pmr_literals;
                ++m_num_literals;
                next = pos;
            }
            previous_was_pmr = pos;
        // Case 4) offset = 0 && length = 1 -> literal
        } else if (offset == 0 && length == 1) {
            previous_was_pmr = 0;
            ++m_num_literals;
            next = pos;
        // Case 1) offset > 0 && length > 1 -> normal match
        } else {
            if (previous_was_pmr > 0) {
                // We cannot have 2 PMRs in a row.. So make this PMR match a normal
                m_cost_array[pos].offset = m_cost_array[pos].pmr_offset;
            }
            next = pos;
            m_num_matched_bytes += length;
            ++m_num_matches;
            previous_was_pmr = 0;
        }
        if (pos == len) {
            m_cost_array[pos].next = 0;
        } 
        m_cost_array[pos-length].next = next;
        pos -= length;
    }


    if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
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
    }
    if (m_lz_config->debug_level > DEBUG_LEVEL_NONE) {
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
    assert(m_cost_array[1].num_literals >= 1);
    pos = 0;

    if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
        std::cerr << "** TANS DEBUG OUTPUT **\n";
    }

    while ((pos = m_cost_array[pos].next)) {
        length = m_cost_array[pos].length;
        offset = m_cost_array[pos].offset;
   
        if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) { 
            std::cerr << "pos: " << std::setw(8) << std::left << pos << " ";
        }
        if (offset == 0 && length == 1) {
            // Literal run
            num_literals = m_cost_array[pos].num_literals;
			sym = m_cost.impl_get_length_bits(num_literals);

            if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                std::cerr << "LITRUN_SYMS " << std::setw(8) << std::right << num_literals 
                          << ":" << std::left << sym << "\n";
            }
            m_cost.inc_tans_symbol_freq(TANS_LITERAL_RUN_SYMS,sym);
            pos = pos + num_literals - 1;
		} else {
            // Match of some kind.. 
			if (length > m_lz_config->max_match) {
				length = m_lz_config->max_match;
			}

			sym = m_cost.impl_get_length_bits(length);
            m_cost.inc_tans_symbol_freq(TANS_LENGTH_SYMS,sym); 
            
			if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                std::cerr << "LENGTH_SYMS " << std::setw(8) << std::right << length 
                          << ":" << std::left << sym;
            }
            if ((offset == 0 && length > 1) || (offset > 0 && length == 1)) {
                // This is a PMR match
                if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                    std::cerr << ", PMR offset\n";
                }
            } else {
                // This is a normal match
                sym = m_cost.impl_get_offset_bits(offset);
				if (offset < m_lz_config->min_offset) {
					sym = 0;
				} else {
					sym = sym - min_offset_bits + 1;	
				}

                m_cost.inc_tans_symbol_freq(TANS_OFFSET_SYMS,sym);
                if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                    std::cerr << ", OFFSET_SYMS " << std::setw(8) << std::right
                              << offset << ":" << std::left << sym << "\n";
                }
            }
        }
    }

    // Build tANS tables
    m_cost.build_tans_tables();
    
    if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
        m_cost.dump(TANS_LITERAL_RUN_SYMS);
        m_cost.dump(TANS_LENGTH_SYMS);
        m_cost.dump(TANS_OFFSET_SYMS);
    }
    return 0;
}


/**
 *
 *
 *
 *
 */

void zxpac4c::preload_tans(int type, uint8_t* freqs, int len)
{
}


const cost* zxpac4c::lz_cost_array_get(int len)
{
    if (len < 1) {
       return NULL;
    }
    lz_cost_array_done();
    m_cost_array = m_cost.alloc_cost(len,m_lz_config->max_chain); 
    m_alloc_len = len;

	// Init tANS symbold freqs.. these could have be preloaded..
    m_cost.set_tans_symbol_freqs(TANS_LITERAL_RUN_SYMS);
    m_cost.set_tans_symbol_freqs(TANS_LENGTH_SYMS);
    m_cost.set_tans_symbol_freqs(TANS_OFFSET_SYMS);

    return m_cost_array;
}

void zxpac4c::lz_cost_array_done(void)
{
    if (m_alloc_len > 0) {
        m_cost.free_cost(m_cost_array); 
    }
    m_alloc_len = 0;
    m_cost_array = NULL;
}

int zxpac4c::encode_history(const char* buf, char* p_out, int len, int pos)
{
    char literal;
    int tag;
    int length;
    int offset;
    int m,n;
    int run_length;
    putbits_history pb(p_out);
    int header_size_to_sub;
    char byte_tag;
	int min_offset_bits = log2(m_lz_config->min_offset);

    m_security_distance = 0;
    
    if (m_lz_config->reverse_encoded) {
        header_size_to_sub = 4;
    } else {
        header_size_to_sub = 0;
    }

    // Build header at the beginning of the file.. max 16M files supported.
    pb.byte(m_lz_config->initial_pmr_offset);
    pb.byte(len >> 16);
    pb.byte(len >> 8);
    pb.byte(len >> 0);
        
    // Insert tANS Ls tables into the output using K=2 bits Rice encoding
    rice_encoder<2> rice;
    const int* syms;
    uint32_t s;
    int sbits;

    offset = pb.size();
    length = 0;



    // *FIX* MUST also encode the last (initial) state into the file

    // Encode literal run table
    syms = m_cost.get_tans_scaled_symbol_freqs(TANS_LITERAL_RUN_SYMS,m);
    length += m;
    for (n = 0; n < m; n++) {
        s = syms[n] & 0xff;
        sbits = rice.encode_value(s);
        pb.bits(s,sbits);
    }

    // Encode match length table
    syms = m_cost.get_tans_scaled_symbol_freqs(TANS_LENGTH_SYMS,m);
    length += m;
    for (n = 0; n < m; n++) {
        s = syms[n] & 0xff;
        sbits = rice.encode_value(s);
        pb.bits(s,sbits);
    }
        
    // Encode Offset table
    syms = m_cost.get_tans_scaled_symbol_freqs(TANS_OFFSET_SYMS,m);
    length += m;
    for (n = 0; n < m; n++) {
        s = syms[n] & 0xff;
        sbits = rice.encode_value(s);
        pb.bits(s,sbits);
    }
    
    pos = pb.size();
    if (m_lz_config->verbose) {
        std::cout << "Encoded " << length << " bytes of tANS tables to "
                  << pos-offset << " bytes\n";
    }

    //
    pos = 0;
    
    while ((pos = m_cost_array[pos].next)) {
        length = m_cost_array[pos].length;
        offset = m_cost_array[pos].offset;
		literal = buf[pos-1];

        if (offset == 0 && length == 1) {
            // encode raw literal run.. note that we adjust pos 
            run_length = m_cost_array[pos--].num_literals;
            
			// pos is now fixed to point into the buf
            if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                std::cerr << "O: Literal run of " << run_length;
            }
            if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                std::cerr << ", bits(0,1), ";
            }
            if (run_length > m_lz_config->max_literal_run) {
                if (m_lz_config->verbose) {
                    std::cout << "**Error: cannot handle literal run > " << m_lz_config->max_literal_run
                              << " bytes\n";
                }
                return -1;
            }
            
            n = m_cost.impl_get_literal_tag(buf+pos,length,byte_tag,tag);

            if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                std::cerr << ", tag 0x"
						  << std::right << std::setw(2) << std::hex << std::setfill('0') << tag 
						  << "," << std::dec << n;
            }
            
            pb.bits(0,1);
            pb.bits(tag,n);

            for (m = 0; m < run_length; m++) {
                pb.byte(buf[pos++]);
                n += 8;
            }
            if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                std::cerr << " ('" << (std::isprint(literal) ? literal : '.');
				std::cerr << "') -> total " << std::dec << 1+n+run_length << "\n";
            }
        } else {
            n = 0;

			if ((offset == 0 && length > 1) || (offset > 0 && length == 1)) {
                if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                    std::cerr << "O: PMR Match, ";
                }
                tag = 0;
            } else {
                tag = 1;
                if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                    std::cerr << "O: Match, ";
                }
            }   
            if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
                std::cerr << "bits("<< tag << ",1), ";
            }
            
			pb.bits(tag,1);
            
			if (tag == 1) {
                // encode offset if this was a normal match
				n = m_cost.get_offset_tag(offset,literal,tag);
				pb.bits(literal,min_offset_bits);
				pb.bits(tag,n);
				n += min_offset_bits;

				if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
					std::cerr << ", ";
				}
            }

            // encode match or PMR match length
			if (length > m_lz_config->max_match) {
                if (m_lz_config->verbose) {
                    std::cout << "**Parsing Error:  match longet than " << m_lz_config->max_match
                              << " bytes\n";
                }
                return -1;
            }

            m = m_cost.get_length_tag(length,tag);
            pb.bits(tag,m);

			if (m_lz_config->debug_level > DEBUG_LEVEL_NORMAL) {
				std::cerr << " -> total " << 1+m+n << "\n";
			}
        }
        

        //
        n = pb.size();

        if (n >= len) {
            return -1;
        }

        n = n - pos - header_size_to_sub;

        if (n > 0) {
            if (n > m_security_distance) {
                m_security_distance = n;
            }
        }
    }

    n = pb.flush() - p_out;
    return n;
}


int zxpac4c::lz_encode(char* buf, int len, char* p_out, std::ofstream* ofs)
{
    int n;
	(void)ofs;

	if (p_out == NULL) {
		std::cerr << ERR_PREAMBLE << "cannot handle directly to file compression" << std::endl;
		return -1;
	}

    n = encode_history(buf,p_out,len,0);
    return n;
}

