
#include "input_capture.hpp"
#include "stm32f030x6.h"
#include "SignalBuffer.hpp"
#include "main.hpp"
#include "util.hpp"
#include "interface.hpp"
#include "sys/printf_getchar.hpp"

#include "storage/flash.hpp"
#include "util.hpp"

namespace ic
{

    void init_ic()
    {

        BOOL_GATE;

        // Power up timer unit
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

        { // PA 6 pin settings

            // Set PA6 into AF mode
            GPIOA->MODER |= (0b10 << GPIO_MODER_MODER6_Pos);

            // select AF1 mode (TIM3_CH1)
            // we want the lower AFR ( AFRL ) since it has pins 0 ... 7
            GPIOA->AFR[0] |= (0b0001 << GPIO_AFRL_AFSEL6_Pos);
        }

        /* timer setup */
        TIM3->CR1 &= ~TIM_CR1_CEN; // disable for configuration

        // count up
        TIM3->CR1 &= ~TIM_CR1_DIR;               // 0 = up counting
        TIM3->CR1 &= ~(0b11 << TIM_CR1_CMS_Pos); // delete CMS to count what it says in direction (UP)

        // set PSC
        TIM3->PSC = 0;
        TIM3->PSC |= 7; // PSC = (PCLK / target_freq) - 1 -> (8M / 1M) - 1 = 7

        // generate update to reload PSC
        TIM3->EGR |= TIM_EGR_UG;

        // no filtering:
        TIM3->CCMR1 &= ~(TIM_CCMR1_IC1F); // clear to 0
        TIM3->CCMR1 |= (0b01 << TIM_CCMR1_CC1S_Pos);

        // set reset mode
        TIM3->SMCR &= ~TIM_SMCR_SMS;               // clear it
        TIM3->SMCR |= (0b100 << TIM_SMCR_SMS_Pos); // reset mode

        // trigger reset on any edge from the edge detector
        TIM3->SMCR &= ~TIM_SMCR_TS;
        TIM3->SMCR |= (0b100 << TIM_SMCR_TS_Pos); // TI1 Edge Detector, meaning both edges

        // we want to detecet all edges

        // enable interrupts
        TIM3->CCER |= TIM_CCER_CC1E;
        TIM3->CCER |= (TIM_CCER_CC1P | TIM_CCER_CC1NP); // capture rising and falling edges

        TIM3->DIER |= TIM_DIER_CC1IE; // capture 1 interrupt enable

        TIM3->DIER |= TIM_DIER_UIE; // enable update interrupts
        TIM3->CR1 |= TIM_CR1_URS;   // only throw update interrupts on overflow

        // enable trigger interrupt

        NVIC_SetPriority(TIM3_IRQn, 0x06); // set priority (0x06 is picked arbitrarily)

        TIM3->CR1 |= TIM_CR1_CEN; // enable the thing

        /**
         * What does it do:
         * - Throw permanent interrupts with 11111
         * - All flags are set, ignore the first 3 because of the channels
         *
         * - why is UF always set
         * - why is CC1F and UF set at the same time ?
         * - the fact that UF and CC1F are set at the same time makes it impossible to differentiate update from capture events should they happen at the same time
         *
         * - changing PA6 logic level does not change anything or cause more or less interrupts, making the thing quasi useless
         *
         */
    }

    void disable_ic(void)
    {
        NVIC_DisableIRQ(TIM3_IRQn);
    }

    void enable_ic(void)
    {
        NVIC_EnableIRQ(TIM3_IRQn);
    }

    auto signalBuffer = sb::SignalBuffer();

#ifdef __cplusplus
    extern "C"
    {
#endif

        // only start processing interrupts after a second has passed
        uint64_t last_time_interrupted = 0;

        void TIM3_IRQHandler(void)
        {

            volatile uint32_t SR = TIM3->SR;
            TIM3->SR = 0;

            static uint32_t overflow_counter = 0;

            if (SR & TIM_SR_CC1IF)
            {
                // calc actual period with overflow offset
                const auto period = TIM3->CCR1 + (overflow_counter << 16);
                overflow_counter = 0;

                if (period >= UINT16_MAX || period < 200)
                {
                    // skip, cannot possibly be relevant
                    // TODO: CHECK if this is actually needed on actual hardware, might be needlessly limiting
                }
                else
                {
                    signalBuffer.push(period);
                    last_time_interrupted = rcc::getSystick();
                }
            }

            if (SR & TIM_SR_CC1OF)
            {
                printf("[Err] overcapture!\n");
            }

            if (SR & TIM_SR_UIF)
            {
                overflow_counter++;
            }
        }

#ifdef __cplusplus
    }
#endif

    uint64_t get_last_time_interrupted()
    {
        return last_time_interrupted;
    }

    int process_signals()
    {

        uint16_t window[8] = {0};
        memset((void *)window, 0, sizeof(window)); // clear the window for every new calc

        // get the current received signals
        if (!signalBuffer.getWindow(window, 8))
        {
            return -1;
        }

        // send_array(window, 8);
        // send("\n");

        // check ever pattern and check for a hit

        for (int period_pattern_index = 0;; period_pattern_index++)
        {
            const auto period_pattern = flash::getPattern(period_pattern_index);

            if (period_pattern == nullptr)
            {
                break;
            }

            if (period_pattern->match_window(window))
            {
                // shift out the signal we hit
                signalBuffer.shift_read_head(period_pattern->getLength());
                return period_pattern_index;
            }
        }

        // shift ahead by 1
        signalBuffer.shift_read_head();

        return -1;
    }

}
