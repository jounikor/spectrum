#ifndef _RADIXSORT3_H_INCLUDED
#define _RADIXSORT3_H_INCLUDED
/**
 *
 * @brief A byte size bucket based radix sort implementation for self re-learning purposes. 
 *
 *        This implementation is using inheritance instead of just overloading the
 *        sort function. A bit more messy but seems to work.
 *
 * @file radixsort3.h
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

template<typename T> class radixsort_8bb: public radix_sort<T> {
    int *p_bucket;
protected:
    using radix_sort<T>::m_bits;
    using radix_sort<T>::m_size;
    using radix_sort<T>::p_tmp;

public:
    radixsort_8bb(int size, int bits=0) throw(std::invalid_argument);
    virtual ~radixsort_8bb(); 
    void sort(T *inout, int size=0) throw(std::out_of_range);
    bool is_valid() const;
};

#endif // _RADIXSORT3_H_INCLUDED
