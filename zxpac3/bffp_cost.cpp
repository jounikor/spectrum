/**
 * @file bffp_cost.cpp
 * @brief Calculates an approximate cost ofr the LZ encoding. This class
 *        is also aware of the used encoding.. and may maintain a state.
 *
 *
 *
 *
 */

#include <iostream>
#include "lz_utils.h"
#include "bffp.h"
#include "bffp_cost.h"



bffp_cost::bffp_cost(int p0, int p1, int p2, int p3, int ins, int res):
    m_pmr(p0,p1,p2,p3),
    m_mtf(256,128,ins,res)
{
    std::cout << "bffp_cost::bffp_cost(): " << p0 << ", " << p1
        << ", " << p2 << ", " << p3 << ", " << ins << ", " << res
        << std::endl;
}

bffp_cost::~bffp_cost()
{
    std::cout << "bffp_cost::~bffp_cost()\n";
}

void* bffp_cost::state_save(void* ctx)
{
    return m_pmr.state_save(ctx);
}

void bffp_cost::state_load(void *ctx)
{
    m_pmr.state_load(ctx);
}

void bffp_cost::init_buffer(const char* buf, int buf_len)
{
    m_buf = buf;
    m_buf_len = buf_len;
}


/**
 * @brief Calculate the cost of this match..
 *
 * @param[in] off  Offset for the match. If offset == 0 then this is a literal or
 *                 a run of literals.
 * @param[in] len  Length of the match. 
 * @return
 *   
 *
 */

int bffp_cost::cost(int off, int len, cost_t& cost_estimate) 
{
    cost_estimate.tag = static_cast<int>(lz_tag_type::none);
    int tag_coding_len = 1;
    int n;
    uint8_t c;

    // Literal.. always prefer PMRx with length 1 over MTF256
    if (off == 0) {
        for (n = 0; n < 4; n++) {
            // len is also the char..
            if (len == m_buf[m_pmr[n]]) {
                cost_estimate.tag = static_cast<int>(lz_tag_type::bffp_pmr0) + n;
                cost_estimate.len = 1;
                cost_estimate.cost = 1;
                m_pmr.update_pmr(n);

                return 0;
            }
        }

        cost_estimate.tag = static_cast<int>(lz_tag_type::bffp_lit);
        
        if (m_mtf.update_mtf(cost_estimate.tag,&c)) {
            cost_estimate.cost = 2;
        } else {
            cost_estimate.cost = 1;
        }
        
        return -1;
    }

    // Match.. 
    // #1 test PMRs

    for (n = 0; n < 4; n++) {
        // len is also the char..
        if (len == m_buf[m_pmr[n]]) {
            cost_estimate.tag = static_cast<int>(lz_tag_type::bffp_pmr0) + n;
            cost_estimate.len = 1;
            cost_estimate.cost = 1;
            return cost_estimate.cost;
        }
    }


    if (len >= 16) {
        ++tag_coding_len;
    }
    // any PMRs?

    return cost_estimate.cost;
}

#if 1
void *get_state(int& len)
{
    return NULL;
}


#endif

