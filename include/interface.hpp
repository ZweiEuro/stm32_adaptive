#pragma once
#include "inttypes.h"
#include "input_capture.hpp"
#include "classes/PeriodPattern.hpp"
#include <queue>
namespace interface
{
    extern std::queue<uint8_t> found_indices;
    /** Uart communication and commands
     *
     */

    enum CMD : uint8_t
    {
        C_IDLE = 0,
        C_SETUP,
        C_START = 's', // start input capture
        C_HALT = 'h',  // halt input capture
        C_TEST = 't',  // send hard-coded test signal
        C_DEV_TEST = 'a',
        C_FADE_TO_COLOR = 'f', // print out all the "found" things

        // END
        M_MAX
    };

    // page content
    // last page: 0x0800 7C00 - 0x0800 7FFF 1 Kbyte Page 31

    // UART

    void handle_usart(void);
}