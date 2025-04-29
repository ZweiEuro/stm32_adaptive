
#include "input_capture.hpp"
#include "stm32f030x6.h"
#include "SignalBuffer.hpp"
#include "main.hpp"
#include "util.hpp"
#include "config.hpp"

namespace ic
{

    void init_ic()
    {

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

        // TODO: remove!
        // only process after a delay
        if (rcc::getSystick() < last_time_interrupted + 1000)
        {
            return -1;
        }

        // send_array(window, 8);
        // send("\n");

        // check ever pattern and check for a hit
        for (int i = 0; i < conf::n_patterns; i++)
        {
            if (conf::period_patterns[i]->match_window(window))
            {
                // shift out the signal we hit
                signalBuffer.shift_read_head(conf::period_patterns[i]->getLength());
                return i;
            }
        }

        // shift ahead by 1
        signalBuffer.shift_read_head();

        return -1;
    }

    // CLASS

    PeriodPattern::PeriodPattern(const uint16_t signal_pattern[PATTERN_MAX_N], uint8_t tolerance)
    {
        _tolerance_mul_256 = tolerance;
        memcpy(periods, signal_pattern, 8 * sizeof(uint16_t));

        _length = PATTERN_MAX_N;
        for (int i = 0; i < 8; i++)
        {
            if (periods[i] == 0)
            {
                _length = i;
                break;
            }
        }
    }

    bool PeriodPattern::match_window(const uint16_t signal_pattern[PATTERN_MAX_N])
    {
        if (_length == 0)
        {
            return false;
        }

        for (int index = 0; index < _length; index++)
        {
            const volatile auto target_val = periods[index];
            const volatile auto signal_period = signal_pattern[index];

            if (target_val == 0.0)
            {
                // we are done
                return true;
            }

            if (signal_period == 0.0)
            {
                return false;
            }

            // fixed point math to spare ourselves the float struggle
            uint32_t lower_bound = target_val * 256 - target_val * _tolerance_mul_256;
            lower_bound /= 256;
            uint32_t upper_bound = target_val * 256 + target_val * _tolerance_mul_256;
            upper_bound /= 256;

            const bool within_tolerance = lower_bound <= signal_period &&
                                          signal_period <= upper_bound;

            if (!within_tolerance)
            {
#if IC_DEBUG
                send("miss: ");
                send(index);
                send(" ");
                send(lower_bound);
                send(" <= ");
                send(signal_period);
                send(" <= ");
                send(upper_bound);
                send('\n');
#endif
                return false;
            }
            else
            {
#if IC_DEBUG
                send("good: ");
                send(index);
                send(" ");
                send(lower_bound);
                send(" <= ");
                send(signal_period);
                send(" <= ");
                send(upper_bound);
                send('\n');
#endif
            }
        }
        return true;
    }

    void PeriodPattern::print()
    {
        send("[PP] l: ");
        send(this->_length);
        send(' ');
        send_array(periods, this->getLength());
        send(" t: ");
        send(this->_tolerance_mul_256);
        send("\n");
    }
}
