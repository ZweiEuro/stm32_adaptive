
#include "sys/printf_getchar.hpp"

#include "util.hpp"

namespace ws2815
{
    uint8_t led_dma_timing_buffer[24 + 1] = {0};

    void setup()
    {

        BOOL_GATE;

        // PIN PA7
        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOAEN); // Enable GPIOA bank

        // PA7 push pull output
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODER7_Pos);

        // PA7 into alternate function 5 -> TIM17_CH1
        SET_BIT(GPIOA->AFR[0], 0b0101 << GPIO_AFRL_AFSEL7_Pos);

        // enable the timer, TIM17_CH1
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM17EN);

        //// no prescaler, 72MHz clock
        TIM17->PSC = 0;
        TIM17->ARR = 12 - 1; // ARR / 8Mhz = 850ns -> ARR = 8Mhz * 850ns

        TIM17->CCR1 = 6 / 2; // unused preload value

        TIM17->BDTR |= TIM_BDTR_MOE;                         // master output enable (use the channel pins as output)
        TIM17->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1; // PWM mode 1
        TIM17->CCMR1 |= TIM_CCMR1_OC1PE;
        TIM17->CCER |= TIM_CCER_CC1E; // compare 1 output enable

        // TIM17->CCER |= TIM_CCER_CC1P; // invert the output due to the hardware inverter

        //  TIM17->DIER |= TIM_DIER_CC1DE; // CC1 DMA

        TIM17->CR1 |= TIM_CR1_CEN;
    }

    void test()
    {
        setup();

        printf("WS2815 test\n");

        while (true)
        {
            SET_BIT(TIM17->CR1, TIM_CR1_CEN);

            util::delay_ms(1);
            CLEAR_BIT(TIM17->CR1, TIM_CR1_CEN);

            util::delay_ms(1);
        }
    }
}