/**
 * @file lz_utils.cpp
 *
 *
 *
 *
 */

#include <iostream>
#include <new>
#include "lz_utils.h"

/**
 *
 * @param size_window[in] Number of match arrays 
 * @param num_matches[in]
 *
 *
 */
matches::matches(int max_forward, int max_chain) 
{
    int m, n;

    std::cout << "matches::matches(" << max_forward <<"," << max_chain <<")\n";

    m_max_forward = max_forward;
    m_max_chain = max_chain;
    m_max_forward_minus_1 = max_forward-1;

    m_pos_matches = NULL;
    m_all_matches = NULL;
 
    try {
        m_all_matches = new match_head[max_forward];
    }
    catch (...) {
        throw;
    }

    try {
        m_pos_matches = new match[max_forward*max_chain];
    }
    catch (...) {
        delete m_all_matches;
        throw;
    }

    for (n = 0, m = 0; n < max_forward; n++) {
        m_all_matches[n].matches = &m_pos_matches[m];
        m_all_matches[n].num_matches = 0;
        m_all_matches[n].max_match_index = -1;
        m += max_chain;
    }
}

matches::~matches(void) 
{
    std::cout << "matches::~matches(void)\n";
    if (m_all_matches) { delete[] m_all_matches; }
    if (m_pos_matches) { delete[] m_pos_matches; }
}

match_head& matches::operator[](int pos)
{
    return m_all_matches[pos & m_max_forward_minus_1];
}

match_head& matches::at(int pos) 
{
    return m_all_matches[pos & m_max_forward_minus_1];
}

const match_head& matches::operator[](int pos) const
{
    return m_all_matches[pos & m_max_forward_minus_1];
}

const match_head& matches::at(int pos) const
{
    return m_all_matches[pos & m_max_forward_minus_1];
}


