/**
 * @file tans.cpp
 * @author Jouni 'Mr.Spiv' korhonen
 * @version 0.1
 * @copyright The Unlicense
 * @brief Test code for tANS encoder.
 * 
 *
 *
 */
#define TRACE_ENABLED

#include <cassert>
#include "ans.h"
#include "tans_encoder.h"


SET_TRACE_LEVEL(INFO)


static const uint8_t Ls[] = {4,3,1,0,0,5,3,1};
static uint8_t S[] = {0,1,0,2,2,0,2,1,2,1,2,1,1,1,1,1,0,1,0,2,1,2,2,2,1,2,0,2,0,2,0,0,0,0,5,5,5,5,5,5,5,5,5,5,5,5,5,6};



int main(void)
{

    tans_encoder<uint8_t,32> *p_tans;

    try {
        TRACE_INFO("Testing info level" << std::endl)
        std::cerr.flush();
        p_tans = new tans_encoder<uint8_t,32>(Ls,sizeof(Ls),TRACE_LEVEL_NONE);

    } catch (std::exception& e) {
        TRACE_DBUG("debug level" << std::endl);
        std::cout << e.what() << std::endl;
        return 0;
    }

    //

    ans_state_t state = p_tans->init_encoder(S[0]);
    TRACE_INFO("Initial state: " << std::hex << state
        << std::dec << std::endl);

    for (int i = 1; i < sizeof(S); i++) {
        uint8_t s = S[i];
        uint8_t k;
        uint32_t b;

        state = p_tans->encode(s,k,b);
        assert(k > 0);
        TRACE_INFO("state: 0x" << std::hex << state
            << ", k: 0x" << static_cast<uint32_t>(k) 
            << ", b: 0x" << b 
            << std::dec << std::endl);
    }

    state = p_tans->done_encoder();
    TRACE_INFO("Final state: 0x" << std::hex << state << std::dec << std::endl)


    delete p_tans;
    return 0;
}
