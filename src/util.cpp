
#include "util.hpp"
#include "rcc.hpp"

namespace util
{

    void delay_ms(int ms)
    {
        uint64_t delay_until = (uint64_t)rcc::getSystick() + (uint64_t)ms;

        while (delay_until > rcc::getSystick())
        {
            ;
        }
    }
}