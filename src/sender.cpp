#include "stm32f030x6.h"

#include "usart.hpp"
#include "sender.hpp"
namespace sender
{

#ifdef __cplusplus
    extern "C"
    {
#endif

        const uint16_t arr[] = {500, 800};

        void TIM14_IRQHandler(void)
        {
            static volatile uint8_t index = 0;

            if (TIM14->SR & TIM_SR_UIF)
            {
                GPIOA->ODR ^= (1 << 0);

                TIM14->ARR = arr[(index++) % 2];
            }

            TIM14->SR = 0;
        }
#ifdef __cplusplus
    }
#endif

    void setup()
    {
        // enable power
        RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;

        // Registers:

        // CR1
        TIM14->CR1 &= ~TIM_CR1_CEN; // disable it for configuration

        // count down
        TIM14->CR1 |= TIM_CR1_DIR;
        TIM14->CR1 &= ~(0b11 << TIM_CR1_CMS_Pos); // do what dir says

        // only interrupt on overflow
        TIM14->CR1 |= TIM_CR1_URS;

        // DIER

        // enable update interrupt
        TIM14->DIER |= TIM_DIER_UIE;

        // SR, EGR
        /* no config */

        // CCMR
        // not needed, we use neither output nor input mode

        // CCER
        // not needed, we are not toggling or doing something with any pins (directly form timer)

        // CNT
        /* no config */

        // PSC
        TIM14->PSC = 0;
        TIM14->PSC |= 7; // PSC = (PCLK / target_freq) - 1 -> (8M / 1M) - 1 = 7

        // load PSC
        TIM14->EGR |= TIM_EGR_UG;

        // ARR
        TIM14->ARR = 0;

        // CCR1
        // neither input nor output

        // OR
        // not needed to reconfigure mapping of pins and/or channels

        // enable compare interrupt
        NVIC_SetPriority(TIM14_IRQn, 0x06); // set priority (0x06 is picked arbitrarily)
        NVIC_EnableIRQ(TIM14_IRQn);
    }

    void send(const ic::PeriodPattern &pattern)
    {
        static bool _setup = false;
        if (!_setup)
        {
            _setup = true;
            setup();
        }
    }
}