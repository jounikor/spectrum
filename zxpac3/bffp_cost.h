#ifndef _BFFP_COST_INCLUDED
#define _BFFP_COST_INCLUDED

#include "lz_base.h"
#include "pmr.h"
#include "mtf256.h"
#include "lz_utils.h"

class bffp_cost: public lz_cost<bffp_cost> {
    int m_off_hi_octet;
    const char* m_buf;
    int m_buf_len;
    pmr m_pmr;
    mtf_encode m_mtf;
public:
    bffp_cost(int,int,int,int,int,int);
    ~bffp_cost(void);

    void init_buffer(const char* buf, int buf_len);

    int cost(int off, int len, cost_t& estimated_cost);
    void* state_save(void *ctx);
    void* get_state(int& len);
    void state_load(void *ct);
};



#endif  // _BFFP_COST_INCLUDED
