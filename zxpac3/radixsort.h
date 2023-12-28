#ifndef _RADIXSORT_H_INCLUDED
#define _RADIXSORT_H_INCLUDED

/**
 *
 * @brief A radix sort base class implementation for self re-learning purposes.
 * @file radixsort1.h
 * @author Jouni 'Mr.Spiv' Korhonen
 * @version 0.1
 * @date 2023
 * @note Part of re-learning exercise for various algorithms. It has been
 *       too long programming anything outside importable Python stuff..
 * @license Unlicense
 *
 */


#include <stdexcept>

#include <cstdint>
#include <cassert>
#include <cctype>
#include <cstring>
#include <fstream>

#define INPLACE_SIZE    0

template<typename T> class radix_sort {
protected:
    int m_bits;
    T* p_tmp;
    int m_size;
    radix_sort(const radix_sort& )=delete;
    T& operator=(const radix_sort&)=delete;

public:
    radix_sort(int size, int bits=0) throw(std::invalid_argument);
    virtual ~radix_sort();
    void sort(T *inout, int size=0) throw(std::out_of_range);
    bool is_valid() const;
};

template<typename T> bool radix_sort<T>::is_valid() const { 
    if (m_size) {
        return p_tmp != NULL;
    } else {
        return true;
    }
}

template<typename T> radix_sort<T>::radix_sort(int size, int bits) throw (std::invalid_argument)
{
    if (size > 0 && size < 2) {
        throw std::invalid_argument("Array size too small");
    }
    if (bits & 1) {
        throw std::invalid_argument("Number of bits cannot be odd");
    }
    if (size == 0) {
        // You better know what you are doing
        p_tmp = NULL;
    } else {
        p_tmp = new(std::nothrow) T[size];
    }

    m_size = size;

    // m_bits must be multiple of 2

    if (bits == 0) { 
        m_bits = sizeof(T) * 8;
    } else {
        m_bits = bits;
    }
}

template<typename T> radix_sort<T>::~radix_sort()
{
    if (p_tmp) {
        delete[] p_tmp;
   }
}

#endif  // _RADIXSORT_H_INCLUDED
