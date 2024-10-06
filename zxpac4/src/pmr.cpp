/**
 * @file pmr.cpp
 * @brief Implementation of 4 stage previous match reference (offsets only) class.
 *
 *
 *
 *
 *
 */

#include <iostream>
#include <cstdio>
#include "pmr.h"


pmr::pmr(pmr_ctx_t p0, pmr_ctx_t p1, pmr_ctx_t p2, pmr_ctx_t p3)
{
    m_p0 = p0; m_p1 = p1; m_p2 = p2; m_p3 = p3;
    reinit();
}

void pmr::reinit(void)
{
    m_context = 0xe4;   // 11 10 01 00
    m_pmr[0] = m_p0;
    m_pmr[1] = m_p1;
    m_pmr[2] = m_p2;
    m_pmr[3] = m_p3;
}

/*
 *
 * index  3 2 1 0
 * slots ddccbbaa -> 2-bit indices to m_pmr[]
 *
 */

void pmr::update(int slot, pmr_ctx_t offset)
{
    if (slot > 0) {
        // move to front
        int shift           = slot << 1;
        uint8_t mask_static = 0xfc << shift;
        uint8_t mask_shift  = (mask_static ^ 0xff) >> 2;
        uint8_t index       = (m_context >> shift) & 3;
        uint8_t temp        = (m_context & mask_shift) << 2;
        temp               |= (m_context & mask_static);
        m_context           = temp | index;
    } else if (slot < 0){
        // add new
        uint8_t last = (m_context >> 6) & 3;
#ifdef PMR_MTF_SECOND_LAST
        uint8_t second_last = (m_context >> 4) & 3;
        m_pmr[last] = m_pmr[second_last];
        m_pmr[second_last] = offset;
#else   // PMR_MTF_SECOND_LAST
        m_pmr[last] = offset;
#endif
    }
}    

pmr_ctx_t* pmr::state_save(pmr_ctx_t *ctx)
{
    pmr_ctx_t* p;
    int n;

    if (ctx == NULL) {
        p = m_pmr_saved;
    } else {
        p = ctx;
    }

    for (n = 0; n < 4; n++) {
        p[n] = m_pmr[n];
    }
    p[n] = m_context;
    
    return p;
}

void pmr::state_load(pmr_ctx_t *ctx)
{
    pmr_ctx_t *p;
    int n;

    if (ctx == NULL) {
        p = m_pmr_saved;
    } else {
        p = ctx;
    }

    for (n = 0; n < 4; n++) {
        m_pmr[n] = p[n];
    }
    m_context = p[n];
}

pmr_ctx_t& pmr::get(int pp)
{
    uint8_t slot = (m_context >> (pp << 1)) & 3;
    return m_pmr[slot];
}

pmr_ctx_t& pmr::operator[](int pp)
{
    uint8_t slot = (m_context >> (pp << 1)) & 3;
    return m_pmr[slot];
}

void pmr::dump(void) const
{
    ::printf("  PMR Context 0->3 = %d:%d:%d:%d\n",
        m_context&3, (m_context>>2)&3, (m_context>>4)&3, (m_context>>6)&3);
    ::printf("  PMR[0]=%-d PMR[1]=%-d PMR[2]=%-d PMR[3]=%-d\n",
        m_pmr[0],m_pmr[1],m_pmr[2],m_pmr[3]);
}


int pmr::find_pmr_by_offset(pmr_ctx_t offset)
{
    for (int pmr_index = 0; pmr_index < PMR_MAX_NUM; pmr_index++) {
        if (offset == get(pmr_index)) {
            return pmr_index;
        }
    }

    return -1;
}




#if 0
int main(int argc, char** argv)
{
    pmr p(1111,2222,3333,4444);

    return 0;
}


#endif
