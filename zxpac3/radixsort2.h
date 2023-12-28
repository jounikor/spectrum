#ifndef _RADIXSORT2_H_INCLUDED
#define _RADIXSORT2_H_INCLUDED
/**
 *
 * @brief An inplace binary radix sort implementation for self re-learning purposes.
 *
 *        This implementation is using inheritance instead of just overloading the
 *        sort function. A bit more messy but seems to work.
 *
 * @file radixsort2.h
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

template<typename T> class inplace: public radix_sort<T> {
protected:
    using radix_sort<T>::m_bits;
    using radix_sort<T>::m_size;
    void sort(T *inout, int sta, int end, int bits);

public:
    inplace(int bits=0) throw(std::invalid_argument);
    virtual ~inplace(); 
    void sort(T *inout, int size=0) throw(std::out_of_range);
    void swap(T&, T&);
};

#endif // _RADIXSORT2_H_INCLUDED
