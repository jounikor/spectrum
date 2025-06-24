/**
 *
 *
 *
 *
 *
 *
 *
 *
 */
#define TRACE_ENABLED

#include "ans.h"
#include "tans_encoder.h"
#include "trace.h"


SET_TRACE_LEVEL(NONE)

int main(void) {

    try {
        TRACE_INFO("Testinf info level" << std::endl)
        std::cerr.flush();
        ans_base tans(31);
        std::cout << tans.get_k_(13,32) << std::endl;
    } catch (std::exception& e) {
        TRACE_DBUG("bedug level")
        std::cout << e.what() << std::endl;
        return 0;
    }


    return 0;
}
