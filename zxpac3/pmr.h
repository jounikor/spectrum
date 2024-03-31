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

// Used for reserving space if someone wants to save the
// current context of the PMR object into external storage
// within the same architecture (i.e. endianess is not taken
// care of).
#define PMR_CTX_SIZE 5*sizeof(int32_t)

class pmr {
    int32_t m_pmr[4];
    uint8_t m_context;
public:
    pmr(int32_t p0, int32_t p1, int32_t p2, int32_t p3);
    ~pmr(void) {}
    void reinit(int32_t p0, int32_t p1, int32_t p2, int32_t p3);

    int& operator[](int pp);
    int& get_pmr(int pp);
    void update_pmr(int slot, int32_t offset=-1);

    void* state_save(void *ctx);
    void state_load(void *ctx);

    void dump(void) const;

};

#endif // _PMR_H_INCLUDED
