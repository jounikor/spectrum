/**
 * @file mtf256.cpp
 * @version 0.3
 * @brief Implements a move to forward class.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 29-Mar-2023
 * @date 23-Jun-2023
 * @copyright The Unlicense
 *
 */

#include <iostream>
#include <iomanip>
#include <new>
#include <cassert>
#include <cctype>
#include <cstring>
#include <fstream>

#include "mtf256.h"

/**
 * @brief Default constructor for the mtf256 class. 
 * @details The default constructor for the mtf256 class, which also allows
 *          for defining the alphabet size (max 256), number of alphabets
 *          that can be encoded using a single octet (can be also 256),
 *          and what is the number of reserved code points for escaping 
 *          the codes that cannot be directly encoded at this point.
 *          If @p size and @p encodeable are the same then no escaping
 *          take place. if @p reserved is greater than 1 then that number
 *          of codes can be encoded using one escape sequence. If @p insert
 *          is greater than 0 it means the escaped code is not moved 
 *          directly into the first slot of the mtf-array allowing slighly
 *          less aggressive insertion of new code points.
 *          It is also possible to control the number of move-to-fronts steps
 *          on each update using the @max_mtf parameter. Value 0 moves the
 *          referenced value always to the first item in the mtf-array.
 *
 * @param[in] size The size of the MTF alphabet. Values between 2 to 256
 *                 are valid.
 * @param[in] encodeable The number of values that can uniquely be encoded into
 *                 the output stream. For example, 128 implies values 0 to 127
 *                 can only be encoded into the output stream,
 * @param[in] insert The position in the MTF array where to insert new escaped
 *                 values. It has to be somewhere between 0 and
 *                 @p size - @p encodeable - 1.
 * @param[in] reserved The number of escape codes to insert new values into the
                   MTF array. The larger @p reserved value the less overhead
                   escape codes generate in the event there is multiple escapes
                   in a row. Keep the value low.
 * @param[in] max_mtf The number of MTF update steps each time a value is 
 *                 references in the MTF array. 0 means move the referenced
 *                 value always to the first element in the MTF array. The lower
 *                 value the faster the update step is but with some efficiency
 *                 penalty.
 *
 * @return    None.
 */
mtf256::mtf256(int size, int encodeable, int insert, int reserved, int max_mtf)
{
    if ((size < 2 || insert < 0 || encodeable < 0 || reserved < 1)
        || (size > 256) || (encodeable > size)
        || (insert >= encodeable-reserved)) {
        throw std::invalid_argument("Invalid values.");
    }

    m_max_mtf = max_mtf;
    m_size = size;
    m_insert = insert;
    m_escape = encodeable == size ? size : encodeable - reserved;
    m_reserved = reserved;
    m_encodeable = encodeable;
    m_arr = new uint8_t[m_escape];
    reinit();
}

void mtf256::reinit(uint8_t *tab)
{
    if (tab) {
        for (uint32_t n = 0; n < m_escape; n++) {
             m_arr[n] = tab[n];
        }
    } else {
        for (uint32_t n = 0; n < m_escape; n++) {
            m_arr[n] = n;
        }
    }
    m_total_bytes = 0;
    m_escaped_bytes = 0;
}

mtf256::~mtf256()
{
    delete[] m_arr;
}

void mtf256::update(int to_nth, int from_nth, uint8_t value)
{
    while (from_nth > to_nth) {
        --from_nth;
        m_arr[from_nth+1] = m_arr[from_nth];
    }

    m_arr[from_nth] = value;
}

bool mtf256::would_escape(uint8_t value)
{
    int from_nth = find_index(value);

    if (from_nth < m_escape) {
        return false;
    } else {
        return true;
    }
}

bool mtf_encode::update_mtf(uint8_t value, uint8_t *found)
{
    int from_nth = find_index(value);
    bool escaped = false;
    int to_nth;

    update_total_bytes();

    if (from_nth < m_escape) {
        *found = from_nth;

        if (m_max_mtf > 0) {
            // Move the referenced value @ref mtf256::m_max_mtf steps
            // toward to the front of the mtf-array.
            to_nth = from_nth - m_max_mtf;
        
            if (to_nth < 0) {
                to_nth = 0;
            }
        } else {
            // Move the referenced value to the first element
            // in the mof-array.
            to_nth = 0;
        }
    } else {
        // In an case there is a need to escape a new value it will
        // be encoded into the outout stream as the value itself not
        // as the index into the mtf-array.
        *found = value;

        to_nth = m_insert;
        from_nth = m_escape - 1;
        escaped = true;
        update_escaped_bytes();
    }

    update(to_nth,from_nth,value);
    return escaped;
}





uint8_t mtf_decode::update_mtf(uint8_t index, bool after_escape)
{
    uint8_t value;
    int to_nth;

    update_total_bytes();

    if (after_escape) {
        value = index;
        index = m_escape - 1;
        to_nth = m_insert;
        update_escaped_bytes();
    } else {
        value = m_arr[index];

        if (m_max_mtf > 0) {
            to_nth = index - m_max_mtf;
        
            if (to_nth < 0) {
                to_nth = 0;
            }
        } else {
            to_nth = 0;
        }
    }
    
    update(to_nth,index,value);
    return value;
}


void mtf256::state_load(void *ctx, int len)
{
}




/*
 *
 *
 */

#define MAX_LIT 5


#if 0


int main(int argc, char** argv)
{
    std::ifstream ifs;
    std::ofstream ofs;
    bool decode = false;
    mtf_encode E(256,128,96,MAX_LIT);
    mtf_decode D(256,128,96,MAX_LIT);

    int queue_size;
    uint8_t queue[MAX_LIT];

    if (argc < 4) {
        ::printf("**Error: no input and output files given\n");
        return 0;
    }

    if (!::strcmp(argv[1],"d")) {
        decode = true;
    }

    ifs.open(argv[2]);
    if (!ifs.is_open()) {
        ::printf("**Error: failed to open '%s'\n",argv[2]);
        return 0;
    }

    ofs.open(argv[3]);
    if (!ofs.is_open()) {
        ifs.close();
        ::printf("**Error: failed to open '%s'\n",argv[3]);
        return 0;
    }

    uint8_t escape = D.get_escape();
    uint8_t result;
    int value;
    bool got_escaped;

    queue_size = 0;
    value = ifs.get();

    if (!decode) {
        while (ifs.good()) {
            if ((got_escaped = E.update_mtf(value, &result))) {
                queue[queue_size++] = result;
            }
            if (queue_size == MAX_LIT || (got_escaped == false && queue_size > 0)) {
                // flush..
                uint8_t escape_many = E.get_escape(queue_size);
                ofs << escape_many;

                for (int n = 0; n < queue_size; n++) {
                    ofs << queue[n];
                }
                
                queue_size = 0;
            }

            if (queue_size == 0 && got_escaped == false) {
                ofs << result;
            }

            value = ifs.get();
        }

        if (queue_size > 0) {
            uint8_t escape_many = E.get_escape(queue_size);
            ofs << escape_many;

            for (int n = 0; n < queue_size; n++) {
                ofs << queue[n];
            }
        }
    } else {
        // Decode here..
        while (ifs.good()) {
            if (value >= escape) {
                queue_size = D.get_num_escaped(value) - 1;
                value = ifs.get();
                got_escaped = true;
            } else {
                got_escaped = false;
                queue_size = 0;
            }

            do {
                result = D.update_mtf(value,got_escaped);
                ofs << (uint8_t)result;
                value = ifs.get();
            } while (queue_size-- > 0);
        }
    }

    ifs.close();
    ofs.close();

    if (decode) {
        printf("Decoded total: %d, escaped: %d\n",D.get_total_bytes(),D.get_escaped_bytes());
    } else {
        printf("Encoded total: %d, escaped: %d\n",E.get_total_bytes(),E.get_escaped_bytes());
    }


    return 0;
}
#endif
