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
#include "tans_decoder.h"


SET_TRACE_LEVEL(INFO)

#define M 32

static const uint8_t Ls[] = {4,3,1,0,0,5,3,1};
static uint8_t S[] = {0,1,0,2,2,0,2,1,2,1,2,1,1,1,1,1,0,1,0,2,1,2,2,2,1,2,0,2,0,2,0,0,0,0,5,5,5,5,5,5,5,5,5,5,5,5,5,6};
//static uint8_t S[] = {0,1,0,2,2,0,2,1,2,1,2,1,1,1,1,1,0,1,0,2,1,2,2,2,1,2,0,2,0,2,0,0,0,0};
static uint8_t b_buf[sizeof(S)];


int main(void)
{

    tans_encoder<uint8_t,M> *p_tans;
    tans_decoder<uint8_t,M> *p_detans;
    uint8_t *p_b = b_buf;
    uint8_t s;
    uint8_t k;
    uint32_t b;

    try {
        TRACE_INFO("Testing info level" << std::endl)
        p_tans = new tans_encoder<uint8_t,M>(Ls,sizeof(Ls),TRACE_LEVEL_DBUG);

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
        s = S[i];

        state = p_tans->encode(s,state,k,b);
        assert(k > 0);
        TRACE_INFO("state: 0x" << std::hex << state
            << ", k: 0x" << static_cast<uint32_t>(k) 
            << ", b: 0x" << b 
            << std::dec << std::endl);
        *p_b++ = b;
    }

    state = p_tans->done_encoder(state);
    TRACE_INFO("Final state: 0x" << std::hex << state << std::dec << std::endl)

    // We need to use already scaled symbol frequencies, since the
    // tand_decoder class does not implement scaleSymbolFreqs().
    p_detans = new tans_decoder<uint8_t,M>(
        p_tans->get_scaled_Ls(),
        p_tans->get_Ls_len(),
        TRACE_LEVEL_NONE);
    
    state = p_detans->init_decoder(state);
    for (int i = 0; i < sizeof(S); i++) { 
        s = p_detans->decode(state,k);
        --p_b;
        state = p_detans->next_state(state,*p_b);
        TRACE_INFO("new state: 0x" << std::hex << state
            << ", s: 0x" << static_cast<uint32_t>(s)
            << ", k: 0x" << static_cast<uint32_t>(k)
            << std::dec << std::endl)
    }

    delete p_tans;
    delete p_detans;
    return 0;
}
