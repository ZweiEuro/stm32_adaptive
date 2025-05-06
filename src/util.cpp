
#include "util.hpp"
#include "rcc.hpp"

namespace util
{

    void delay_ms(int ms)
    {
        uint64_t delay_until = (uint64_t)rcc::getSystick() + (uint64_t)ms;

        while (delay_until > rcc::getSystick())
        {
            __WFI();
        }
    }

    void toggle_onboard()
    {
        const auto PIN_PA4_Pos = (1 << 4);

        static bool setup = false;
        if (!setup)
        {
            setup = true;

            // power on the pin bank
            RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // enable the clock to GPIO

            // PA4
            GPIOA->MODER |= GPIO_MODER_MODER4_0; // set pins to be general purpose output
        }

        GPIOA->ODR ^= PIN_PA4_Pos;
    }
}