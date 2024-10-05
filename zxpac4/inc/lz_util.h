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


class putbits {
public:
    virtual ~putbits(void) { }
    virtual char* bits(int value, int num_bits) = 0;
    virtual char* flush(void) = 0;
    virtual char* byte(int byte) = 0;
};


class putbits_history : public putbits {
    int m_bb;
    int m_bc;
    char* m_bitbuf_tag_ptr;
    char* m_bitbuf_ptr;
public:
    putbits_history(char* p_buf) {
        m_bb = 0;
        m_bc = 8;
        m_bitbuf_tag_ptr = NULL;
        m_bitbuf_ptr = p_buf;
    }
    ~putbits_history(void) { flush();  }
    char* bits(int value, int num_bits) {
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
    char* flush(void) {
        char* p_old_pos = m_bitbuf_ptr;
        if (m_bc < 8) {
            *m_bitbuf_tag_ptr = m_bb << m_bc;
            m_bitbuf_tag_ptr = m_bitbuf_ptr++;
            m_bc = 8;
        }
        return p_old_pos;
    }
    char* byte(int byte) {
        char* p_old_pos = m_bitbuf_ptr;
        *m_bitbuf_ptr++ = byte & 0xff;
        return p_old_pos;
    }
};

#if 1

class putbits_forward : public putbits {
    int m_bb;
    int m_bc;
    char* m_bitbuf_ptr;
public:
    putbits_forward(char* p_buf) {
        m_bb = 0;
        m_bc = 8;
        m_bitbuf_ptr = p_buf;
    }
    ~putbits_forward(void) {
        flush();
    }
    char* bits(int value, int num_bits) {
        if (num_bits > 8) {
            // put low 8 bits
            bits(value & 0xff,8);
            // and then high order remaining bits
            num_bits -= 8;
            value >>= 8;
        }
        
        char* p_old_pos = m_bitbuf_ptr;
        m_bb = (m_bb << num_bits) | value;

        if (num_bits >= m_bc) {
            num_bits -= m_bc;
            *m_bitbuf_ptr++ = m_bb >> num_bits;
            m_bc = 8;
        }
          
        m_bc -= num_bits;
        return p_old_pos;
    }
    char* flush(void) {
        char* p_old_pos = m_bitbuf_ptr;
        if (m_bc < 8) {
            *m_bitbuf_ptr++ = m_bb << m_bc;
            m_bc = 8;
        }
        return p_old_pos;
    }
    char* byte(int byte) {
        char* p_old_pos = m_bitbuf_ptr;
        *m_bitbuf_ptr++ = byte & 0xff;
        return p_old_pos;
    }
};
#endif


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
