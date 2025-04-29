#pragma once

#include "classes/PeriodPattern.hpp"

namespace flash
{

#ifdef __cplusplus
    extern "C"
    {
#endif
        // comes from the linker and cannot be mangled

        // Storage space for all the signal information
        extern uint8_t __SEC_SIGNAL_PATTERNS_DATA_START[1024];

        extern uint8_t __SEC_SYSTEM_RUN_DATA_START[1024];

#ifdef __cplusplus
    }
#endif

    /**
     * Organization __SEC_SIGNAL_PATTERNS_DATA_START:
     *
     * After a magic byte, follows the number of period patterns inside the list.
     * Then the binary information is listed
     *
     *
     * Magic bytes:
     * __SEC_SIGNAL_PATTERNS_DATA_START[0] = 0xAB
     * __SEC_SIGNAL_PATTERNS_DATA_START[1] = number of period patterns that follow (16 bit is too many anyways)
     *
     * for i in range(0,n):
     *      for s in range(0,8):
     *          __SEC_SIGNAL_PATTERNS_DATA_START[2 + i*sizeof(PeriodPattern) + s *2 + 0] = MSB
     *          __SEC_SIGNAL_PATTERNS_DATA_START[2 + i*sizeof(PeriodPattern) + s *2 + 1] = LSB uint16_t pattern value
     *      __SEC_SIGNAL_PATTERNS_DATA_START[2 + i*sizeof(PeriodPattern) + 16] = tolerance * 256
     *
     *
     *
     */

#define MBYTE 0xAB

    /**
     * Get pattern at index n
     * returns nullptr should that index not exist OR no pattern exist at all
     */
    PeriodPattern *getPattern(int n);

    /**
     * Expects p to be a pointer aimed at a memory region with `n` back to back structs of
     * PeriodPattern information.
     */
    void savePatterns(PeriodPattern *p, int n);

}