/**
 * @file z_alg.cpp
 * @brief A Z-array algorithm implemntation for LZ string matching.
 * @version 0.1
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
#include "lz_utils.h"

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
z_array::z_array(int len, int min, int max)
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
    m_num = 0;
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
 * @param[in] max_matches  The size of the @matches array. 
 * @param[in] matches      A ptr to array of match objects for collecting matches.
 * @return None.
 */

void z_array::init_get_matches(int max_matches, match *matches = NULL)
{
    //m_z[0] = 0;
    m_num = 0;
    m_m = matches;
    m_size = max_matches;
}

/**
 * @brief The number of recorded matches in the match vector.
 * @param None.
 * @return The number of recorded matches.
 *
 * If the ptr to std::vector<match> passed to \ref z_array::init() was NULL
 * then matches are not recoreded and the function returns 0.
 */
int z_array::get_num_found_matches(void) const
{
    return m_num;
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
 * @return 0 if no match was found, otherwise, index to the
 *         longest found match.
 *
 * @note This Z-array implementation is more or less a textbook example
 *       of a possible implementation.
 */
int z_array::find_matches(const char *buf, int len, bool only_better_matches=false)
{
    int max = m_min-1;
    int idx = -1;
    int p, t, i, L=0, R=0;
    int mm = m_min-1;

    /* If the length of the buffer is greater that the Z-array
     * shorten the search to Z-array length.
     */
    if (len > m_len) {
        len = m_len;
    }

    for (i = 1; i < len; i++) {
        p = 0;          // == R-L
        
        if (i > R) {
            L = R = i;
            
            while (R < len && p < m_max && buf[p] == buf[R]) {
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
                
                while (R < len && p < m_max && buf[p] == buf[R]) {
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
                idx = m_num;

                if (only_better_matches) {
                    mm = max;
                }
            }
            if (m_m && m_num < m_size) {
                m_m[m_num].pos = i;
                m_m[m_num++].len = p;

                // If number of recorded matches has been reached stop searching.
                if (m_num == m_size) {
                    break;
                }
            }
        }
    }

    return idx;
}

/* testing stuff */

#if 0
#include "bffp.h"       // just for testing


int main(int argc, char **argv)
{
    const char *buf = argv[1];
    int len = ::strlen(buf);
    int j, n,i;
    int mmm = 3;

    printf("min_match_length: %d\n",bffp::min_match_length_s);
    printf("num_window_sizes: %d\n",bffp::num_window_sizes_s);
    for (n = 0; n < bffp::num_window_sizes_s; n++) {
        printf("> %d\n",bffp::window_sizes_s[n]);
    }


    matches all_matches(len,mmm);
    match_head mh;
    match *m;

    z_array *lz = new z_array(50,2,6);

    for (j = 0; j < len; j++) {
        mh = all_matches[j];
        m = mh.matches;

        lz->init_get_matches(all_matches.get_max_matches(),m);
        i = lz->find_matches(buf+j,len-j,true);

        printf("%2d: '%c' ",j,buf[j]);

        if (i > 0) {
            printf("%2d num %2d\n",i,lz->get_num_matches());
            for(int k = 0; k < lz->get_num_matches(); k++) {
                printf("\tpos %d, len %d\n",m[k].pos,m[k].len);
            }
            mh.max_match_index = i;
            mh.num_matches = lz->get_num_matches();
        } else {
            // Not matches.. offset = 0, length = 1 i.e. the current octet.
            m[0].pos = 0;
            m[0].len = 1;
            mh.num_matches = 0;
            printf("%2d %2d\n",i,0);
        }
    }

    delete lz;
    return 0;
}

#endif

