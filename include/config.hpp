#include "inttypes.h"
#include "input_capture.hpp"

namespace conf
{
    /**
     * Sequence:
     * - SETUP
     * - 8 bit number designating amount of patterns n
     * - n times:
     *   - 8 times, 16 bit values (split into 8 bit packets) for the different periods
     *      - If two hex values are transferred, 0xAA 0xBB then the resulting 16 bit number will be = 0xAABB
     *   - 1 time , 8  bit value designating tolerance t, denoted 1/t
     *
     *
     *
     *
     */

    extern int n_patterns; // how many patterns there are
    extern ic::PeriodPattern **period_patterns;

    enum CMD : uint8_t
    {
        C_IDLE = 0,
        C_SETUP,
        C_PRINT,

        // END
        M_MAX
    };

    // page content
    // last page: 0x0800 7C00 - 0x0800 7FFF 1 Kbyte Page 31

    // UART

    void handle_usart(void);
}