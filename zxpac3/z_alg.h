/**
 * @file z_alg.h
 * @brief A Z-array algorithm implemntation for LZ string matching.
 * @version 0.1
 * @author Jouni 'Mr.Spiv' Korhonen
 * @date 11-Apr-2022
 * @copyright The Unlicense
 *
 * This file implements a Z-array class for serching matches. It is tailored
 * towards serving as a generic building block for LZ string matching engine.
 */

#ifndef _Z_ALG_H_INCLUDED
#define _Z_ALG_H_INCLUDED

#include "lz_utils.h"


/**
 * @brief A class to create a Z-array for a LZ string matching engine.
 * @class z_array z_alg.hxx
 *
 *
 *
 */

class z_array {
    int32_t *m_z;       ///< Pointer to Z-array 
    int m_len;          ///< Length of the Z-array
    int m_max;          ///< Maximum prefix length for matches
    int m_min;          ///< Minimum prefix length for matches
    int m_num;          ///< Number of recorded matches
    int m_size;         ///< Size of the m_m array.
    match *m_m;         ///< Pointer to collected matches.. may be NULL
public:
    virtual ~z_array();
    z_array(int window_len, int min_match, int max_match);

    int find_matches(const char *, int, bool);
    void init_get_matches(int, match *);
    int get_num_found_matches(void) const;
    int get_window(void) const;

    // These could be protected
    int& operator[](int);
    const int32_t& operator[](int) const;
    z_array& operator=(const z_array &) = delete;

    // For iterators use
    int32_t *begin(void);
    const int32_t *end(void) const;
    int32_t *end(void);
    const int32_t *begin(void) const;
};

#define Z_ARRAY_MAXSIZE 0x7fffffff

#endif      // _Z_ALG_H_INCLUDED
