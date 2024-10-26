/**
 * @file lz_util.h
 *
 *
 *
 *
 *
 *
 */

#ifndef _LZ_UTIL_NOT_INCLUDED
#define _LZ_UTIL_NOT_INCLUDED

#include <iostream>
#include <exception>

#define EXCEPTION(exp,str) throw(exp(std::string(__FILE__) + ":" +  \
            std::to_string(__LINE__) + " " + #str))

struct match {
    int32_t offset;                ///< Relative position of a match to the prefix string
    int32_t length;                ///< Length of a match
};


template<typename derived> class putbits {
    derived& impl(void) {
        return *static_cast<derived*>(this);
    }
public:
    ~putbits(void) { }
    char* bits(int value, int num_bits) {
        return impl().impl_bits(value,num_bits);
    }
    char* flush(void) {
        return impl().impl_flush();
    }
    char* byte(int byte) {
        return impl().impl_byte(byte);
    }
    int size(void) {
        return impl().impl_size();
    }
};


class putbits_history : public putbits<putbits_history> {
    int m_bb;
    int m_bc;
    char* m_bitbuf_tag_ptr;
    char* m_bitbuf_ptr;
    char* m_start_ptr;
public:
    putbits_history(char* p_buf) {
        m_bb = 0;
        m_bc = 8;
        m_bitbuf_tag_ptr = NULL;
        m_bitbuf_ptr = p_buf;
        m_start_ptr = p_buf;
    }
    ~putbits_history(void) { impl_flush();  }
    char* impl_bits(int value, int num_bits) {
        int t;

        if (m_bitbuf_tag_ptr == NULL) {
            m_bitbuf_tag_ptr = m_bitbuf_ptr++;
        }
        if (num_bits > 8) {
            t = num_bits - 8;
            // put remaining high order bits
            bits(value>>8,t);
            // and then low order 8 bits
            num_bits = 8;
            value &= 0xff;
        }
        
        char* p_old_pos = m_bitbuf_ptr;   
        m_bb = (m_bb << num_bits) | value;

        //if (num_bits >= m_bc) {
        if (num_bits > m_bc) {
            t = num_bits - m_bc;
            *m_bitbuf_tag_ptr = m_bb >> t;
            m_bitbuf_tag_ptr = m_bitbuf_ptr++;
            m_bc = 8;
            num_bits = t;
        }
          
        m_bc -= num_bits;
        return p_old_pos;
    }
    char* impl_flush(void) {
        char* p_old_pos = m_bitbuf_ptr;
        if (m_bc < 8) {
            *m_bitbuf_tag_ptr = m_bb << m_bc;
            m_bitbuf_tag_ptr = m_bitbuf_ptr++;
            m_bc = 8;
        }
        return p_old_pos;
    }
    char* impl_byte(int byte) {
        char* p_old_pos = m_bitbuf_ptr;
        *m_bitbuf_ptr++ = byte & 0xff;
        return p_old_pos;
    }
    int impl_size(void) {
        return m_bitbuf_ptr - m_start_ptr;
    }
};


/**
 *
 *
 *
 *
 *
 *
 *
 */
template<typename derived> class lz_match {
    derived& impl(void) {
        return *static_cast<derived*>(this);
    }
public:
    virtual ~lz_match(void) {}
    int find_matches(const char *buf, int pos, int len , bool only_better) {
        return impl().impl_find_matches(buf,pos,len,only_better);
    }
    void init_get_matches(int len, match *matches) {
        impl().impl_init_get_matches(len,matches);
    }
    void reinit(void) {
        impl().impl_reinit();
    }
};


#endif  // _LZ_UTIL_NOT_INCLUDED
