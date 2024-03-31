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


pmr::pmr(int32_t p0, int32_t p1, int32_t p2, int32_t p3)
{
    reinit(p0,p1,p2,p3);
}

void pmr::reinit(int32_t p0=4, int32_t p1=8, int32_t p2=12, int32_t p3=16)
{
    m_context = 0xe4;   // 11 10 01 00
    m_pmr[0] = p0;
    m_pmr[1] = p1;
    m_pmr[2] = p2;
    m_pmr[3] = p3;
}

/*
 *
 * index  3 2 1 0
 * slots ddccbbaa -> 2-bit indices to m_pmr[]
 *
 */

void pmr::update_pmr(int slot, int32_t offset)
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

void* pmr::state_save(void *ctx = NULL)
{
    int32_t* p;
    int n;

    if (ctx == NULL) {
        p = new (std::nothrow) int32_t[5];
    } else {
        p = static_cast<int32_t*>(ctx);
    }

    for (n = 0; n < 4; n++) {
        p[n] = m_pmr[n];
    }
    p[n] = static_cast<int32_t>(m_context);
    
    return p;
}

void pmr::state_load(void *ctx)
{
    int32_t *p = static_cast<int32_t*>(ctx);
    int n;

    for (n = 0; n < 4; n++) {
        m_pmr[n] = p[n];
    }
    m_context = static_cast<uint8_t>(p[n]);
}

int& pmr::get_pmr(int pp)
{
    uint8_t slot = (m_context >> (pp << 1)) & 3;
    return m_pmr[slot];
}

int& pmr::operator[](int pp)
{
    return get_pmr(pp);
}

void pmr::dump(void) const
{
    ::printf("Context 0: %d, 1: %d, 2: %d, 3: %d\n",
        m_context&3, (m_context>>2)&3, (m_context>>4)&3, (m_context>>6)&3);
    ::printf("PMR[0]=%6d, PMR[1]=%6d, PMR[2]=%6d, PMR[3]=%6d\n",
        m_pmr[0],m_pmr[1],m_pmr[2],m_pmr[3]);
}


#if 0
int main(int argc, char** argv)
{
    pmr p(1111,2222,3333,4444);

    return 0;
}


#endif
