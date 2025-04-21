
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

    auto signalBuffer = sb::SignalBuffer(500);

#ifdef __cplusplus
    extern "C"
    {
#endif
        uint64_t last_time_interrupted = 0;
        bool done = false;

        //*************************************************************************************
        void TIM3_IRQHandler(void)
        {

            volatile uint32_t SR = TIM3->SR;
            TIM3->SR = 0;

            if (done)
            {
                return;
            }

            static uint32_t overflow_counter = 0;

            if (SR & TIM_SR_CC1IF)
            {
                const auto period = TIM3->CCR1 + (overflow_counter << 16);

                // discard the value if its larger than 16 bit can hold
                if (period < UINT16_MAX)
                {
                    signalBuffer.push(period);
                    last_time_interrupted = rcc::getSystick();
                }
                overflow_counter = 0;
            }

            if (SR & TIM_SR_UIF)
            {
                overflow_counter++;
            }
        }
        //*************************************************************************************

// Declarations of this file
#ifdef __cplusplus
    }
#endif

    int process_signals()
    {

        uint16_t window[8] = {0};
        memset((void *)window, 0, sizeof(window)); // clear the window for every new calc

        if (!signalBuffer.getWindow(window, 8))
        {
            return 0;
        }

        if (rcc::getSystick() < last_time_interrupted + 1000)
        {
            return 0;
        }

        done = true;

        for (int i = 0; i < conf::n_patterns; i++)
        {

            if (conf::period_patterns[i]->match_window(window))
            {

                signalBuffer.shift_read_head(conf::period_patterns[i]->getLength() - 1);
                send("found Signal: ");
                send(i);
                send(" ");
                send_array(window, 8);
                send('\n');
                return i;
            }
        }

        send("found nothing ");
        send(" ");
        send_array(window, 8);
        send('\n');

        signalBuffer.shift_read_head();

        return 0;
    }

    // CLASS

    PeriodPattern::PeriodPattern(const uint16_t signal_pattern[PATTERN_MAX_N], float tolerance)
    {
        _tolerance = tolerance;
        memcpy(periods, signal_pattern, 8 * sizeof(uint16_t));

        _length = PATTERN_MAX_N;
        for (int i = 0; i < 8; i++)
        {
            if (periods[i] == 0)
            {
                _length = i + 1;
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

            const uint32_t lower_bound = target_val * (1.0 - _tolerance);
            const uint32_t upper_bound = target_val * (1.0 + _tolerance);

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
        send("[PP] ");
        send_array(periods, this->getLength());
        send(" t: ");
        send(this->_tolerance);
        send("\n");
    }
}

/*
// left button
found Signal: 0 [308, 11424, 311, 1143, 308, 1139, 310, 1145]
found Signal: 1 [311, 1143, 308, 1139, 310, 1145, 987, 458]
found Signal: 2 [310, 1145, 987, 458, 304, 1150, 302, 1143]
found Signal: 1 [304, 1150, 302, 1143, 305, 1142, 308, 1146]
found Signal: 1 [305, 1142, 308, 1146, 302, 1148, 307, 1145]
found Signal: 1 [302, 1148, 307, 1145, 303, 1144, 305, 1141]
found Signal: 1 [303, 1144, 305, 1141, 304, 1153, 304, 1145]
found Signal: 1 [304, 1153, 304, 1145, 303, 1146, 305, 1144]
found Signal: 1 [303, 1146, 305, 1144, 303, 1153, 301, 1150]
found Signal: 1 [303, 1153, 301, 1150, 300, 1145, 987, 463]
found Signal: 2 [300, 1145, 987, 463, 300, 1155, 987, 463]
found Signal: 2 [300, 1155, 987, 463, 299, 1152, 982, 464]
found Signal: 2 [299, 1152, 982, 464, 297, 11455, 295, 1156]

found Signal: 0 [307, 11421, 312, 1143, 309, 1132, 316, 1137]
found Signal: 1 [312, 1143, 309, 1132, 316, 1137, 992, 454]
found Signal: 2 [316, 1137, 992, 454, 308, 1146, 307, 1139]
found Signal: 1 [308, 1146, 307, 1139, 306, 1141, 305, 1139]
found Signal: 1 [306, 1141, 305, 1139, 307, 1148, 305, 1139]
found Signal: 1 [307, 1148, 305, 1139, 307, 1144, 302, 1144]
found Signal: 1 [307, 1144, 302, 1144, 303, 1152, 300, 1138]
found Signal: 1 [303, 1152, 300, 1138, 307, 1144, 303, 1146]
found Signal: 1 [307, 1144, 303, 1146, 300, 1152, 300, 1144]
found Signal: 1 [300, 1152, 300, 1144, 303, 1147, 983, 459]
found Signal: 2 [303, 1147, 983, 459, 303, 1150, 986, 458]
found Signal: 2 [303, 1150, 986, 458, 302, 1148, 300, 1144]
found Signal: 1 [302, 1148, 300, 1144, 303, 11427, 309, 1154]


 */