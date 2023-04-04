/**
 * @file mtf256list.cpp
 * @version 0.1
 * @brief Implements a move to forward class.
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 29-Mar-2023
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

#include "mtf256list.h"

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
 *
 * @param[in] size The size of the MTF alphabet. Values between 2 to 256
 *                 are valid.
 * @param[in] encodeable
 * @param[in] insert 
 * @param[in] reserved
 *
 * @return
 */
mtf256::mtf256(int size, int encodeable, int insert, int reserved)
    throw(std::invalid_argument,std::range_error,std::bad_alloc)
{
    if ((size < 2 || insert < 0 || encodeable < 0 || reserved < 1)
        || (size > 256) || (encodeable > size)
        || (insert >= encodeable-reserved)) {
        throw std::invalid_argument("Invalid values.");
    }

    m_size = size;
    m_insert = insert;
    m_escape = encodeable == size ? size : encodeable - reserved;
    m_reserved = reserved;
    m_encodeable = encodeable;
    reinit();
}

void mtf256::reinit(void)
{
    for (uint32_t n = 0; n < m_size; n++) {
        // prep circular linked list
        m_next[n] = (n + 1) % m_size;
        m_prev[n] = (n - 1) % m_size;
    }

    // head of the list
    m_head = 0;

    m_total_bytes = 0;
    m_escaped_bytes = 0;
}

void mtf256::update(int to_nth, int from_nth, int from_node)
{
    int to_node;

    if (to_nth == from_nth) {
        return;
    }
    
    to_node = find_nth_node(to_nth);

    if (from_nth < m_escape) {
        m_head = from_node;
    }

    // remove right from the list..
    m_prev[m_next[from_node]] = m_prev[from_node];
    m_next[m_prev[from_node]] = m_next[from_node];
    
    // put right as the first node of the list
    m_prev[from_node] = m_prev[to_node];
    m_next[from_node] = to_node;
    
    m_next[m_prev[to_node]] = from_node; 
    m_prev[to_node] = from_node;
}


mtf256::~mtf256()
{
}


bool mtf_encode::update_mtf(uint8_t value, uint8_t *found) throw(std::out_of_range)
{
    int from_nth;
    int from_node = find_node_by_value(value,&from_nth);
    bool escaped = false;
    int to_nth;

    *found = from_nth;

    update_total_bytes();

    if (from_nth < m_escape) {
        to_nth = 0;
    } else {
        to_nth = m_insert;
        escaped = true;
        update_escaped_bytes();
    }

    update(to_nth,from_nth,from_node);
    return escaped;
}





uint8_t mtf_decode::update_mtf(uint8_t from_nth, bool after_escape) throw(std::out_of_range)
{
    int from_node = find_nth_node(from_nth);
    int to_nth;

    update_total_bytes();

    if (after_escape) {
        to_nth = m_insert;
        update_escaped_bytes();
    } else {
        to_nth = 0;
    }

    update(to_nth,from_nth,from_node);
    return static_cast<uint8_t>(from_node);
}



/*
 *
 *
 */

#define MAX_LIT 5



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

    printf("Total: %d, escaped: %d\n",E.get_total_bytes(),E.get_escaped_bytes());


    return 0;
}







