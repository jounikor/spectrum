/**
 * @file bfhp.cpp
 * @brief A Brute Force Forward Parser for LZ engines..
 * @version 0.1
 * @author Jouni Korhonen
 * @date 5-Feb-2024
 *
 *
 *
 */

#define LZ_PARSE_DEBUG
#define NUM_ESCAPES 3


#include <iosfwd>
#include <fstream>
#include <new>
#include <cstring>
#include "bfhp.h"
#include "bfhp_cost.h"
#include "lz_util.h"

#ifdef LZ_PARSE_DEBUG
#include <cctype>
#endif  // LZ_PARSE_DEBUG

/** 
 * Initialized statics that describe this LZ parser.
 */
const int bfhp::min_match_length_s = 2;
const int bfhp::max_match_length_s = 256;
const int bfhp::num_window_sizes_s = sizeof(bfhp::window_sizes_s)/sizeof(int);
const int bfhp::window_sizes_s[2] = {1<<16, 1<<24};
const int bfhp::max_buffer_length_s = 0;
const bool bfhp::is_forward_parser_s = true;

bfhp::bfhp(int window_size, int min_match, int max_match,
    int max_chain, int backward_steps,
    int ins, int res, int max, 
    int offset_ctx,
    bool default_pmr) :
    lz_base(),
    m_lz(window_size,min_match,max_match),  // may throw exception
    m_cost(DEFAULT_P0,DEFAULT_P1,DEFAULT_P2,DEFAULT_P3,
        ins, res, max, offset_ctx,  backward_steps),
    m_cost_array(NULL),
    m_mtf(256,128,ins,res,max),
    m_default_pmr(default_pmr)
{
    printf("bfhp::bfhp()\n");

    //if (min_match < min_match_length_s) {
    //    //EXCEPTION(std::length_error,"Minimum match length too small");
    //}
    if (max_match > max_match_length_s) {
        //EXCEPTION(std::length_error,"Maximum match length too big");
    }

    switch (window_size) {
    case 1<<16:
    case 1<<24:
        break;
    default:
        EXCEPTION(std::invalid_argument,"Invalid sliding window size");
    }

    m_min_match    = min_match;
    m_max_match    = max_match;
    m_window_size  = window_size;
    m_alloc_len  = 0;
    m_max_chain = max_chain;
    m_offset_ctx = offset_ctx;
    m_debug = false;
    m_verbose = false;
}

bfhp::~bfhp(void)
{
}


int bfhp::lz_search_matches(const char* buf, int len, int interval)
{
    int pos = 0;
    int num;

    // Unused at the moment..
    (void)interval;

    if (m_debug) {
        std::cerr << "-- Find all maches ------------------------------------------\n";
    }

    while (pos < len) {
        m_lz.init_get_matches(m_max_chain,m_cost_array[pos].matches);
            
        // Find all matches at this position. Returned 'num' is the
        // the number of found matches.
        num = m_lz.find_matches(buf,pos,len-pos,false);
        m_cost_array[pos].num_matches = num;

        if (m_debug) {
            // I must say I prefer C formatting over C++ formatting a lot!
            std::cerr << std::setw(6) << std::dec << std::setfill(' ') << pos << ": '"
                << (std::isprint(buf[pos]) ? buf[pos] : ' ')
                << "' (" << std::setw(2) << std::setfill('0') << std::hex << (buf[pos] & 0xff)
                << std::dec << std::setw(0) <<  ") -> ";
            if (num > 0) {
                for (int n = 0; n < num; n++) {
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


int bfhp::lz_parse(const char* buf, int len, int interval)
{
    int length;
    int pos;
    cost_base* cb = static_cast<cost_base*>(m_cost_array);

    // Unused at the moment..
    (void)interval;

    // Select most frequest short PMRs
    m_pmr_offsets[0] = DEFAULT_P0;
    m_pmr_offsets[1] = DEFAULT_P1;
    m_pmr_offsets[2] = DEFAULT_P2;
    m_pmr_offsets[3] = DEFAULT_P3;

    if (m_default_pmr == false) {
        m_cost.calc_initial_pmr8(m_cost_array,m_pmr_offsets,len);
    }
    
    m_cost.pmr_reset(m_pmr_offsets);

    if (m_debug) {
        std::cerr << "-- PMRs calculation -----------------------------------\n";
    
        for (int n = 0; n < PMR_MAX_NUM; n++) {
            std::cerr << "PMR" << n << " offset proposal: " << m_pmr_offsets[n] << "\n";
        }
    }

    pos = 0;
    m_cost.init_cost(cb,0,len,true);

    while (pos < len) {
        // always do literal cost calculation
        m_cost.impl_literal_cost(pos,m_cost_array,buf);

        // match cost calculation if not at the end of file and there was a match
        if (pos < (len - 2)) {
            m_cost.impl_match_cost(pos,m_cost_array,buf);
        }        
        ++pos;
    }

    //
    pos = len;
    int num_literals = 1;
    
    // Fix the links of selected cost nodes
    while (pos > 0) {
        length = m_cost_array[pos].length;
        assert(length > 0);

        if (m_cost_array[pos].tag == lz_tag_type::bfhp_lit) {
            m_cost_array[pos].num_literals = num_literals++;
        } else {
            num_literals = 1;
        }
        if (pos == len) {
            m_cost_array[pos].next = 0;
        } 
        m_cost_array[pos-length].next = pos;
        pos -= length;
    }

    assert(m_cost_array[1].num_literals >= 1);

    if (m_debug) {
        std::cerr << "-- Cost debugging phase ----------------------------------\n";
        pos = 1;

        while (pos < len+1) {
            std::cerr << std::setw(6) << std::setfill(' ') << pos << ": " << m_cost_array[pos].tag << " '"
                << (std::isprint(m_cost_array[pos].literal) ? m_cost_array[pos].literal : ' ')
                << "' (" << std::hex << std::setfill('0') << std::setw(2) << (m_cost_array[pos].literal & 0xff)
                << ") " << std::dec << std::setw(0) << std::setfill(' ') << m_cost_array[pos].num_literals 
                << " " << m_cost_array[pos].offset
                << ":" << m_cost_array[pos].length << "\t" << m_cost_array[pos].arrival_cost
                << " -> " << m_cost_array[pos].next << "\n";
            ++pos;
        }
    
        std::cerr << "-- Show final selected -----------------------------------\n";
        pos = 1;

        while (pos > 0) {
            std::cerr << std::setw(6) << pos << ": " << m_cost_array[pos].tag << "' "
                << (std::isprint(m_cost_array[pos].literal) ? m_cost_array[pos].literal : ' ')
                << "' (" << std::hex << std::setfill('0') << std::setw(2) << (m_cost_array[pos].literal & 0xff)
                << ") " << std::dec << std::setfill(' ') 
                << m_cost_array[pos].num_literals << " " <<  m_cost_array[pos].offset
                << ":" << m_cost_array[pos].length << "\t" << m_cost_array[pos].arrival_cost
                << "\n";
            pos = m_cost_array[pos].next;
        }
    }
    return 0;
}

cost_base* bfhp::lz_cost_array_get(int len)
{
    if (len < 1) {
       return NULL;
    }

    if (len < m_alloc_len && m_cost_array) {
        return m_cost_array;
    }

    m_cost_array = static_cast<cost_with_ctx*>(m_cost.alloc_cost(len,m_max_chain)); 
    m_alloc_len = len;

    return m_cost_array;
}

void bfhp::lz_cost_array_done(void)
{
    m_cost.free_cost(m_cost_array); 
    m_alloc_len = 0;
    m_cost_array = NULL;


}

int bfhp::lz_encode(const cost_base* costs, uint32_t len, std::ofstream& ofs)
{
    const cost_with_ctx* p_res = static_cast<const cost_with_ctx*>(costs);
    int n, m, pos;
    int compressed_size;
    int ctrl;
    char buf[16+8];
    char queue[MTF256_MAX_QUEUE];
    int queue_size;
    int max_queue_size;

    // Reinitialize the MTF256 for literals..
    m_mtf.reinit();
    max_queue_size = m_mtf.get_max_escaped();

    // Construct and output header..
    ctrl = p_res[1].num_literals;
    n = ctrl;
    assert(ctrl > 0);
    assert(m_pmr_offsets[0] < 256);
    assert(m_pmr_offsets[1] < 256);
    assert(m_pmr_offsets[2] < 256);
    assert(m_pmr_offsets[3] < 256);

    if (ctrl > 8) {
        ctrl = 8;
    }

    // control byte
    //
    //   7 6 5 4 3 2 1 0 
    //  +-+-+-+-+-+-+-+-+
    //  |0|0|0|0|0|NRLIT|
    //  +-+-+-+-+-+-+-+-+
    //
    m = 0;

    // 4 octet header in big endian..
    ctrl = (ctrl-1) | (len << 8);
    buf[m++] = ctrl >> 24;
    buf[m++] = ctrl >> 16;
    buf[m++] = ctrl >> 8;
    buf[m++] = ctrl >> 0;

    // 4 octet of initial PMR offsets..
    buf[m++] = m_pmr_offsets[0];
    buf[m++] = m_pmr_offsets[1];
    buf[m++] = m_pmr_offsets[2];
    buf[m++] = m_pmr_offsets[3];

    // Copy NRLIT literals
    pos = 1;

    while (n-- > 0 && pos > 0) {
        buf[m++] = p_res[pos].literal;
        pos = p_res[pos].next;
    }

    assert(pos > 0);
    ofs.write(buf,m);
    compressed_size = m;

    // Compressed data follows..

    n = 0;
    queue_size = 0;

    while (pos > 0) {
        uint8_t code;
        uint8_t len1;
        uint8_t len2;
        int offset_len = 0;
        int offset = p_res[pos].offset;
        int length = p_res[pos].length;
        bool got_escaped = false;

        if (p_res[pos].length > 15) {
            len1 = 0;
            len2 = length & 0xff;
        } else {
            len1 = length & 0xff;
        }

        code = len1;

        switch (p_res[pos].tag) {
        case lz_tag_type::bfhp_lit:
            // tag: 0LLLLLLL
            if ((got_escaped = m_mtf.update_mtf(p_res[pos].literal,&code))) {
                queue[queue_size++] = code;
            }
            len1 = 1;
            offset_len = -1;
            break;
        case lz_tag_type::bfhp_pmr3:
            // tag: 1011nnnn
            code |= 0xb0;
            break;
        case lz_tag_type::bfhp_pmr2:
            // tag: 1010nnnn
            code |= 0xa0;
            break;
        case lz_tag_type::bfhp_pmr1:
            // tag: 1001nnnn
            code |= 0x90;
            break;
        case lz_tag_type::bfhp_pmr0:
            // tag: 1000nnnn
            code |= 0x80;
            break;
        case lz_tag_type::bfhp_mml2_off_ctx:
            // tag: 1100nnnn
            code |= 0xc0;
            offset_len = 1;
            break;
        case lz_tag_type::bfhp_mml2_off8:
            // tag: 1101nnnn
            code |= 0xd0;
            offset_len = 1;
            break;
        case lz_tag_type::bfhp_mml2_off16:
            // tag: 1110nnnn
            code |= 0xe0;
            offset_len = 2;
            break;
        case lz_tag_type::bfhp_mml2_off24:
            // tag: 1111nnnn
            code |= 0xf0;
            offset_len = 3;
            break;
        default:
            std::cerr << "Unknown tag " << p_res[pos].tag << " at pos " << pos << "\n";
            assert(0);
        }

        //
        // offset_len is
        //  -1 for Literal
        //   0 for PMR
        //  >0 for match
        //

        if (queue_size == max_queue_size || (got_escaped == false && queue_size > 0)) {
            // Literal run + escape
            uint8_t escape_many = m_mtf.get_escape(queue_size);
            ofs << escape_many;

            for (int n = 0; n < queue_size; n++) {
                ofs << queue[n];
            }

            compressed_size = compressed_size + queue_size + 1;
            queue_size = 0;
        }
        if (queue_size == 0 && got_escaped == false) {
            // Non-escaped literal or match
            ofs << code; 
            ++compressed_size;
        }
        if (offset_len >= 0) {
            if (len1 == 0) {
                // Extra length byte
                ofs << len2;
             ++compressed_size;
            }
            
            switch (offset_len) {
            case 1:
                buf[0] = offset;
                break;
            case 2:
                buf[0] = offset >> 8; 
                buf[1] = offset;
                break;
            case 3:
                buf[0] = offset >> 16; 
                buf[1] = offset >> 8; 
                buf[2] = offset;
                break;
            default:
                break;
            }
        
            if (offset_len > 0) {
                ofs.write(buf,offset_len);
                compressed_size += offset_len;
            }
        }

        pos = p_res[pos].next;
    }

    if (m_verbose) {
        std::cout << std::dec << std::setw(0);
        std::cout << "Number of ESCAPED literals: " << m_mtf.get_escaped_bytes() << "\n";
        std::cout << "Number of Literals: " << m_mtf.get_total_bytes() << "\n";
    }
    return compressed_size;
}

void bfhp::lz_enable_debug(bool enable)
{
    m_debug = enable;
    m_cost.enable_debug(enable);
}

void bfhp::lz_enable_verbose(bool enable)
{
    m_verbose = enable;
    m_cost.enable_verbose(enable);
}

#if 0


int main(int argc, char **argv)
{
    const char *buf = argv[2];
    int len = ::strlen(buf);
    int n;
    int chain = 8;

    (void)argc;

    std::ifstream ifs;
    std::ofstream ofs(argv[1],std::ios::binary);

    if (!ofs.is_open()) {
        ::printf("Opening file '%s' failed\n",argv[1]);
        return 0;
    }

    printf("min_match_length: %d\n",bfhp::min_match_length_s);
    printf("num_window_sizes: %d\n",bfhp::num_window_sizes_s);
    for (n = 0; n < bfhp::num_window_sizes_s; n++) {
        printf("> %d\n",bfhp::window_sizes_s[n]);
    }

    bfhp *bf = new bfhp(65536,2,256, chain,2, 96,NUM_ESCAPES,1, 256);
    //bfhp *bf = new bfhp(65536,2,256, chain,1, 96,1,0, 256);
    cost_base* my_cost = bf->lz_cost_array_get(len);
    bf->lz_enable_debug(true);

    bf->lz_search_matches(buf,len,0); 
    bf->lz_parse(buf,len,0); 
    n = bf->lz_encode(my_cost,len,ofs); 
    
    bf->lz_cost_array_done();

    ofs.close();

    delete bf;
    return 0;
}

#endif
