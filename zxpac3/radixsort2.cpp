/**
 *
 * @brief An inplace binary radix sort implementation for self re-learning purposes.
 *
 *        This implementation is using inheritance instead of just overloading the
 *        sort function. A bit more messy but seems to work. The template only works
 *        for integer types.
 *        The algorithm uses a signle MSB check and recursion.
 * 
 * @todo  Use of iterators instead of arrays..
 *
 * @file radixsort2.cpp
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

#include "radixsort2.h"

template<typename T> void inplace<T>::swap(T& a, T& b)
{
    T t = a;
    a = b;
    b = t;
}

template<typename T> inplace<T>::inplace(int bits) throw(std::invalid_argument) :
    radix_sort<T>(INPLACE_SIZE,bits)
{
}

template<typename T> inplace<T>::~inplace()
{
}

/**
 * @brief An inplace implementation of a bitwise LSB radix sort algorithm.
 * @param[inout] p_inout A ptr to a buffer of type Ts to sorted. The sorted 
 *                       Ts are also returned in the same buffer.
 * @param[in] size       The number of element in the \p p_inout. If not given 
 *                       then the size given in the constructor is used.
 * 
 * @return none
 *
 */

template<typename T> void inplace<T>::sort(T* p_inout, int sta, int end, int shft)
{
    int first = sta;
    int last = end;
    T mask = 1 << shft;

    while (sta < end) {
        if (p_inout[sta] & mask) {
            swap(p_inout[sta],p_inout[--end]);
        } else {
            ++sta;
        }
    }

    if (shft > 0) {
        if (sta+1 > first) {
            sort(p_inout,first,sta,shft-1);
        }
        if (sta+1 < last) {
            sort(p_inout,sta,last,shft-1);
        }
    }
}

template<typename T> void inplace<T>::sort(T* p_inout, int size) throw(std::out_of_range)
{
    sort(p_inout,0,size,m_bits-1);
}



uint32_t tmp[] = {23423,62342,6234,34,634,134,62,34,2623,42452,121,423,1};

int main(int argc, char **argv)
{
    inplace<uint32_t> S;

    if (S.is_valid()) {
        std::cout << "Is OK!\n" ;
    } else {
        std::cout << "Is NOT ok..\n"; 
    }

    S.sort(tmp,sizeof(tmp)/sizeof(uint32_t));

    for (int n = 0; n < sizeof(tmp)/sizeof(uint32_t); n++) {
        std::cout << n << ":\t" << tmp[n] << std::endl;
    }



    return 0;
}
