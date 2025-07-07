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


SET_TRACE_LEVEL(INFO)


static const uint8_t Ls[] = {4,3,1,0,0,5,3,1};




int main(void) {

    try {
        TRACE_INFO("Testing info level" << std::endl)
        std::cerr.flush();
        tans_encoder<uint8_t,32> tans(reinterpret_cast<const uint8_t*>(Ls),sizeof(Ls),TRACE_LEVEL_NONE);
    } catch (std::exception& e) {
        TRACE_DBUG("debug level" << std::endl);
        std::cout << e.what() << std::endl;
        return 0;
    }


    return 0;
}
