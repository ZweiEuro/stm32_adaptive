
#pragma once
#include "rcc.hpp"

#define BOOL_GATE                   \
    do                              \
    {                               \
        static bool __gate = false; \
        if (__gate)                 \
        {                           \
            return;                 \
        }                           \
        else                        \
        {                           \
            __gate = true;          \
        };                          \
    } while (false)

namespace util
{

    void delay_ms(int ms);

    void toggle_onboard();
}