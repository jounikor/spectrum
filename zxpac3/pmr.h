/**
 * @file pmr.h
 * @brief IMplementation of 4 stage previous match reference (offsets only) class.
 *
 *
 *
 *
 */
 #ifndef _PMR_H_INCLUDED
 #define _PMR_H_INCLUDED

#include <cstdint>


#define PMR_MTF_SECOND_LAST
#define PMR_MAX_NUM 4 
typedef int32_t pmr_ctx_t;

// Used for reserving space if someone wants to save the
// current context of the PMR object into external storage
// within the same architecture (i.e. endianess is not taken
// care of).
#define PMR_CTX_SIZE (PMR_MAX_NUM+1)

class pmr {
    pmr_ctx_t m_pmr[PMR_MAX_NUM];
    pmr_ctx_t m_pmr_saved[PMR_CTX_SIZE];
    pmr_ctx_t m_context;
    pmr_ctx_t m_p0,m_p1,m_p2,m_p3;
public:
    pmr(pmr_ctx_t p0, pmr_ctx_t p1, pmr_ctx_t p2, pmr_ctx_t p3);
    ~pmr(void) {}
    void reinit(void);

    pmr_ctx_t& operator[](int pp);
    pmr_ctx_t& get(int pp);
    void update(int slot, pmr_ctx_t offset=-2);

    pmr_ctx_t* state_save(pmr_ctx_t *ctx=NULL);
    void state_load(pmr_ctx_t *ctx=NULL);

    void dump(void) const;
    int find_pmr_by_offset(pmr_ctx_t offset);
};

#endif // _PMR_H_INCLUDED
