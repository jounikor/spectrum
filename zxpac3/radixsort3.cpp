/**
 *
 * @brief An 8-bit size bucket radix sort implementation for self re-learning purposes.
 * @file radixsort3.cpp
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

#include "radixsort3.h"

template<typename T> radixsort_8bb<T>::radixsort_8bb(int size, int bits) throw(std::invalid_argument) :
    p_bucket(NULL), radix_sort<T>(size,bits)
{
    if (bits % 16) {
        // The requirement for 16-bit bitsize comes from the buffer swapping during
        // sorting. In order to return the resultr in the "input" buffer the sorting
        // steps must be done multiple of 2 times.
        throw std::invalid_argument("Number of bits must be a multiple of 16");
    }
    
    p_bucket = new(std::nothrow) int[256];
}

template<typename T> radixsort_8bb<T>::~radixsort_8bb() 
{
    if (p_bucket) {
        delete[] p_bucket;
    }
}

template<typename T> bool radixsort_8bb<T>::is_valid() const
{
    if (p_bucket == NULL) {
        return false;
    }

    return radix_sort<T>::is_valid();
}


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

template<typename T> void radixsort_8bb<T>::sort(T* p_inout, int size) throw(std::out_of_range)
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

    for (int n = 0; n < m_bits; n += 8) {
        int m;

        // count buckets..
        ::memset(p_bucket,0,sizeof(*p_bucket)*256);

        for (m = 0; m < size; m++) {
            p_bucket[(p_a[m] >> n) & 0xff]++;
        }
        
        // cumulative counts..
        for (m = 1; m < 256; m++) {
            p_bucket[m] += p_bucket[m-1];
        }

        // sort per bucket
        for (int m = size-1; m >= 0; m--) {
            p_b[--p_bucket[(p_a[m] >> n) & 0xff]] = p_a[m];
        }

        // swap buffers
        p_tmp = p_a;
        p_a = p_b;
        p_b = p_tmp;
    }
}

#if 1

uint32_t tmp[] = {23423,62342,6234,34,634,134,62,34,2623,42452,121,423,1};

int main(int argc, char **argv)
{
    radixsort_8bb<uint32_t> S(sizeof(tmp)/sizeof(uint32_t));

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
