#include "stm32f030x6.h"

#include "sender.hpp"
#include "stdlib.h"
#include "interface.hpp"
#include "storage/flash.hpp"
#include "sys/printf_getchar.hpp"
#include "util.hpp"
namespace sender
{

    const auto PIN_PA0_Pos = (1 << 0);

    bool _sending = false; // are we currently processing something

    uint8_t *_pattern_indices = nullptr; // indices into conf::period_patterns of what to send
    uint8_t _indices_array_length = 0;

    uint8_t _current_pattern_indices_index = 0; // indicates the NEXT period to load on interrupt

    uint8_t _current_pattern_period_index = 0;

#ifdef __cplusplus
    extern "C"
    {
#endif

        void TIM14_IRQHandler(void)
        {

            const auto SR = TIM14->SR;
            TIM14->SR = 0;

            if (SR & TIM_SR_UIF)
            {

                auto current_period = flash::getPattern(_pattern_indices[_current_pattern_indices_index]);

                if (_current_pattern_period_index >= current_period->getLength())
                {
                    _current_pattern_period_index = 0;
                    _current_pattern_indices_index++;

                    if (_current_pattern_indices_index >= _indices_array_length)
                    {
                        // are are DONE
                        TIM14->CR1 &= ~TIM_CR1_CEN;
                        _sending = false;
                        return;
                    }
                    else
                    {
                        // get the next one
                        current_period = flash::getPattern(_pattern_indices[_current_pattern_indices_index]);
                    }
                }

                GPIOA->ODR ^= PIN_PA0_Pos;
                TIM14->ARR = current_period->periods[_current_pattern_period_index];
                _current_pattern_period_index++;
            }
        }
#ifdef __cplusplus
    }
#endif

    void setup()
    {

        BOOL_GATE;

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

    void send_434(const uint8_t pattern_indices[], uint8_t length)
    {
        ic::disable_ic();
        setup();

        // copy over the indices that are to be sent
        _pattern_indices = (uint8_t *)calloc(sizeof(uint8_t), length);
        memcpy(_pattern_indices, pattern_indices, sizeof(uint8_t) * length);
        _indices_array_length = length;

        // make sure we start at 0
        _current_pattern_indices_index = 0; // load 0 on pos 1 next
        _current_pattern_period_index = 1;

        // load first value manually
        TIM14->ARR = flash::getPattern(_current_pattern_indices_index)->periods[0];

        // wait until done
        _sending = true;

        GPIOA->ODR |= PIN_PA0_Pos; // turn "off"

        TIM14->CR1 |= TIM_CR1_CEN;

        while (_sending)
            __asm("nop");

        free(_pattern_indices);
        _pattern_indices = nullptr;
        _indices_array_length = 0;

        ic::enable_ic();
    }
}