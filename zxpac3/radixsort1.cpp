/**
 *
 * @brief A binary radix sort implementation for self re-learning purposes.
 * @file radixsort.cpp
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 * @date 2023
 * @note Part of re-learning exercise for various algorithms. It has been
 *       too long programming anything outside importable Python stuff..
 * @license Unlicense
 *
 */



#include <iostream>
#include <iomanip>
#include <new>
#include <stdexcept>

#include <cstdint>
#include <cassert>
#include <cctype>
#include <cstring>
#include <fstream>

#include "radixsort.h"

/**
 * @brief The default sort implementation is a bitwise LSB radox sort algorithm.
 * @param[inout] p_inout A ptr to a buffer of type Ts to sorted. The sorted 
 *                       Ts are also returned in the same buffer.
 * @param[in] size       The number of element in the \p p_inout. If not given 
 *                       then the size given in the constructor is used.
 * 
 * @return none
 *
 */

template<typename T> void radix_sort<T>::sort(T* p_inout, int size) throw(std::out_of_range)
{
    if (size > m_size) {
        throw std::out_of_range("Array too big");
    }
    if (size == 0) {
        size = m_size;
    }

    T* p_a = p_inout;
    T* p_b = p_tmp;
    T* p_tmp;
    int b = 1;

    for (int n = 0; n < m_bits; n++) {
        int c0s = 0;
        int c1s;

        // count numberof 0s and 1s
        for (int m = 0; m < size; m++) {
            if (!(p_a[m] & b)) {
                ++c0s;
            }
        }

        c1s = size;

        // sort per bucket
        for (int m = size-1; m >= 0; m--) {
            if (p_a[m] & b) {
                p_b[--c1s] = p_a[m];
            } else {
                p_b[--c0s] = p_a[m];
            }
        }

        // swap buffers
        p_tmp = p_a;
        p_a = p_b;
        p_b = p_tmp;
    
        // advance to the next bit
        b <<= 1;
    }
}

#if 1

uint32_t tmp[] = {23423,62342,6234,34,634,134,62,34,2623,42452,121,423,1};

int main(int argc, char **argv)
{
    radix_sort<uint32_t> S(sizeof(tmp)/sizeof(uint32_t));

    if (S.is_valid()) {
        std::cout << "Is OK!\n" ;
    } else {
        std::cout << "Is NOT ok..\n"; 
    }


    S.sort(tmp);

    for (int n = 0; n < sizeof(tmp)/sizeof(uint32_t); n++) {
        std::cout << n << ":\t" << tmp[n] << std::endl;
    }

    return 0;
}
#endif
