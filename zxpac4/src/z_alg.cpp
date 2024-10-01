/**
 * @file z_alg.cpp
 * @brief A Z-array algorithm implemntation for LZ string matching.
 * @version 0.2
 * @author Jouni 'Mr.Spiv' Korhonen
 * @copyright The Unlicense
 * @date 11-Apr-2022
 *
 * This file implements a Z-array class for searching matches. It is tailored
 * towards serving as a generic building block for LZ string matching engine.
 */

#include <iostream>
#include <new>

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <cstdlib>

#include "z_alg.h"

/**
 * @brief Return the start of the Z-array for iterators use.
 * @param None
 * @return A ptr to the object internal Z-array.
 * @note The returned pointer is not const.
 */
int32_t *z_array::begin(void)
{
    return m_z;
}

/**
 * @brief Return the start of the Z-array for iterators use.
 * @param None
 * @return A const ptr to the object internal Z-array.
 */
const int32_t *z_array::begin(void) const
{
    return m_z;
}

/**
 * @brief Return the end of the Z-array for iterators use.
 * @param None
 * @return A ptr to the end of the object internal Z-array.
 * @note The returned pointer is not const.
 */
int32_t *z_array::end(void)
{
    return &m_z[m_len];
}

/**
 * @brief Return the end of the Z-array for iterators use.
 * @param None
 * @return A const ptr to the end of the object internal Z-array.
 */
const int32_t *z_array::end(void) const
{
    return &m_z[m_len];
}

/**
 * @brief An overloaded array operator for the Z-array class.
 * @param[in] i Index into the Z-array, which is the start
 *  of the prefix string at position i.
 * @return The length of the prefix string starting at position i.
 */
const int32_t& z_array::operator[](int i) const
{
    return m_z[i];
}

/**
 * @brief An overloaded array operator for the Z-array class.
 * @param[in] i Index into the Z-array, which is the start
 *  of the prefix string at position i.
 * @return The length of the prefix string starting at position i.
 */
int32_t& z_array::operator[](int i)
{
    return m_z[i];
}

/**
 * @brief z_array constructor.
 * 
 * Contruct a z_array object of given Z-array length (also works as a maximum
 * length for the optimal parsing window), minimum match length and maximum
 * match length.
 *
 * @param[in] len Length of the Z-array i.e. the size of search window.
 * @param[in] min Minimum match length.
 * @param[in] max Maximum match length.
 *
 * @return None.
 *
 */
z_array::z_array(int len, int min, int max, int good_match,
    int mm2_thres_offset,
    int mm3_thres_offset)
{
    //if (len > max_window_size_s) {
    if (len > Z_ARRAY_MAXSIZE) {
        EXCEPTION(std::out_of_range,": Sliding window size");
    }

    // no try catch here..
    m_z = new int32_t[len];
    m_len = len;
    m_max = max;
    m_min = min;
    m_good = good_match;
    m_min_match2_threshold_offset = mm2_thres_offset;
    m_min_match3_threshold_offset = mm3_thres_offset;
}

z_array::~z_array(void) 
{
    std::cout << "z_array::~z_array(void)\n";
    delete[] m_z;
}

/**
 * @brief Initialize Z-array for the next iteration.
 * 
 * Initialize the Z-array for use. In case a vector for matches is
 * passed to the method then up to vector size best matches are
 * also collected.
 *
 * @param[in] max_matches  The size of the \a matches array. 
 * @param[in] matches      A ptr to array of match objects for collecting matches.
 * @return None.
 */

void z_array::init_get_matches(int max_matches, match *matches = NULL)
{
    m_m = matches;
    m_size = max_matches;
}

int z_array::get_window(void) const
{
    return m_len;
}

/**
 * @brief Create a Z-array from input of a given length.
 *
 * Create a Z-array of maximum size m_len for a given input buffer.
 * The buffer is treated so that the 'prefix string' to search for is
 * at the position 0 of the input buffer. The searching for matching
 * prefixes start at the position 1 of the input buffer.
 *
 * If a ptr to matches vector array was given at the time of init of the
 * Z-array then found matches are alse stored into the match array.
 * Matches of increasing length are stored until the end of Z-array
 * has been reached or the maximum number of matches to store has
 * been reached.
 *
 * If a ptr to macthes vector array was NULL then the longest match
 * is searched from entire Z-array and an index to it is returned.
 *
 * @param[in] buf A ptr to the buffer to look for matches.
 * @param[in] len Length of the input buffer.
 *
 * @return 0 if no match was found, otherwise, number of found matches.
 *
 * @note This Z-array implementation is more or less a textbook example
 *       of a possible implementation.
 */
int z_array::find_matches(const char *buf, int pos, int len, bool only_better_matches=false)
{
    int max = m_min-1;
    int p, t, i, L=0, R=0;
    int mm = m_min-1;
    int num = 0;

    /* If the length of the buffer is greater that the Z-array
     * shorten the search to Z-array length.
     */
    if (len > m_len) {
        len = m_len;
    }
    
    m_z[0] = 0;

    for (i = 1; i < len; i++) {
        p = 0;          // == R-L
        
        if (i > R) {
            L = R = i;
            
            while (R < len && p < m_max && buf[pos+p] == buf[pos+R]) {
                ++R;
                ++p;
            }

            m_z[i] = p;
            --R;
        } else {
            t = i-L;
        
            if (m_z[t] <= R-i) {
                m_z[i] = m_z[t];
            } else {
                L = i;
                
                while (R < len && p < m_max && buf[pos+p] == buf[pos+R]) {
                    ++R;
                    ++p;
                }
               
                m_z[i] = p;
                --R;
            }
        }

        if (p > mm) {
            if (p > max) {
                max = p;

                if (only_better_matches) {
                    mm = max;
                }
            }
            if (num < m_size) {
                m_m[num].offset = i;
                m_m[num++].length = p;

                // If number of recorded matches has been reached stop searching.
                if (num == m_size) {
                    break;
                }
            }
        }
    }

    return num;
}


