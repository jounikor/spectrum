#ifndef _MTF_H_INCLUDED_
#define _MTF_H_INCLUDED_

#include <cstdint>
#include <exception>

class mtf256 {
    int m_total_bytes;
    int m_escaped_bytes;
protected:
    uint8_t* m_arr;
    int m_size;         ///< Size of the MTF alphabet. Max 256.
    int m_escape;       ///< Edge-reseerved i.e. value to add a new code into MTF.
    int m_reserved;
    int m_insert;       ///< Where to add the new code in the MTF.
    int m_encodeable;   ///< Maximum encodeable code in the output stream.
    int m_max_mtf;      ///< Number of MTF moves done on each update; 0 = to the front.

    int find_index(uint8_t x)
    {
        int index;

        for (index = 0; index < m_escape; index++) {    // was m_size
            if (x == m_arr[index]) {
                break; 
            }
        }
        return index;
    }
    void update(int, int, uint8_t);
    mtf256( int size=256,           ///< Size of the alphabet; max 256.
            int encodeable=128,     ///< How many values can be encoded into output.
            int insert=100,         ///< Position to insert new escaped values.
            int reserved=1,         ///< How many values of 'encodeable' are reserved for escape codes.
            int max_mtf=0           ///< How many steps of MTF is done; 0=all the way to the first.
            ) throw(std::invalid_argument,std::range_error,std::bad_alloc);
    virtual ~mtf256(void);
public:
    void reinit(void);
    
    // Stats
    int get_total_bytes(void) const { return m_total_bytes; }
    int get_escaped_bytes(void) const { return m_escaped_bytes; }
    void update_total_bytes(void) { ++m_total_bytes; }
    void update_escaped_bytes(void) { ++m_escaped_bytes; }

    // Helper methods
    int get_num_escaped(uint8_t token) const {
        return token < m_escape ? 0 : (int)(m_encodeable - token); }
    uint8_t get_escape(int number=0) const {
        return number == 0 ? m_escape : m_escape + m_reserved - number; 
    }

};


class mtf_encode : public mtf256
{

public:
    mtf_encode(int size, int decodeable, int insert, int reserved=1, int max_mtf=0)
        throw(std::invalid_argument,std::range_error,std::bad_alloc):
        mtf256(size, decodeable, insert, reserved, max_mtf) {};
    ~mtf_encode() {};
    bool update_mtf(uint8_t, uint8_t*) throw(std::out_of_range);
};

class mtf_decode : public mtf256
{

public:
    mtf_decode(int size, int decodeable, int insert, int reserved=1, int max_mtf=0)
        throw(std::invalid_argument,std::range_error,std::bad_alloc):
        mtf256(size, decodeable, insert, reserved, max_mtf) {};
    ~mtf_decode() {};
    uint8_t update_mtf(uint8_t, bool) throw(std::out_of_range);
};


#endif  // _MTF_H_INCLUDED_
