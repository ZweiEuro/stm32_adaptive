
#include "input_capture.hpp"
#include "stm32f030x6.h"
#include "usart.hpp"

namespace ic
{

    void init_ic()
    {
        // Power up timer unit
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

        { // PA 6 pin settings

            // Set PA6 into AF mode
            GPIOA->MODER |= (0b11 << GPIO_MODER_MODER6_Pos);

            // select AF1 mode (TIM3_CH1)
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

        // set reset mode
        TIM3->SMCR &= ~TIM_SMCR_SMS;               // clear it
        TIM3->SMCR |= (0b100 << TIM_SMCR_SMS_Pos); // reset mode

        // trigger reset on any edge from the edge detector
        TIM3->SMCR &= ~TIM_SMCR_TS;
        TIM3->SMCR |= (0b100 << TIM_SMCR_TS_Pos); // TI1 Edge Detector, meaning both edges

        // enable interrupts
        TIM3->CCER |= TIM_CCER_CC1E; // capture compare interrupts
        TIM3->DIER |= TIM_DIER_CC1IE;

        TIM3->DIER |= TIM_DIER_UIE; // enable update interrupts
        TIM3->CR1 |= TIM_CR1_URS;   // only throw update interrupts on overflow

        NVIC_SetPriority(TIM3_IRQn, 0x06); // set priority (0x06 is picked arbitrarily)
        NVIC_EnableIRQ(TIM3_IRQn);

        TIM3->CR1 |= TIM_CR1_CEN; // enable the thing

        send("inited IC\n");
    }

#ifdef __cplusplus
    extern "C"
    {
#endif

        //*************************************************************************************
        void TIM3_IRQHandler(void)
        {
            volatile uint32_t SR = TIM3->SR;
            static uint32_t prev_SR = 0;
            TIM3->SR = 0;

            if (SR != prev_SR)
            {

                send("SR: ");
                send_bin(SR);
                send("\n");
            }

            GPIOA->ODR ^= (1 << 4);

            prev_SR = SR;
        }
        //*************************************************************************************

// Declarations of this file
#ifdef __cplusplus
    }
#endif
}
