
#include "sys/printf_getchar.hpp"

#include <cstring>
#include "util.hpp"

namespace ws2815
{
    uint8_t led_dma_timing_buffer[24 + 1] = {0};

    const auto PIN_PA7_Pos = (1 << 7);

    void
    setup_PWM()
    {

        BOOL_GATE;

        // PIN PA7
        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOAEN); // Enable GPIOA bank

        // PA7 into alternate function mode
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODER7_Pos);

        GPIOA->ODR |= (PIN_PA7_Pos);

        // PA7 into alternate function 5 -> TIM17_CH1
        SET_BIT(GPIOA->AFR[0], 0b0101 << GPIO_AFRL_AFSEL7_Pos);

        // enable the timer, TIM17_CH1
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM17EN);

        //// no prescaler, 72MHz clock
        TIM17->PSC = 0;
        TIM17->ARR = 12 - 1; // ARR / 8Mhz = 850ns -> ARR = 8Mhz * 850ns

        TIM17->CCR1 = 0; // initial preload value

        TIM17->BDTR |= TIM_BDTR_MOE;                         // master output enable (use the channel pins as output)
        TIM17->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1; // PWM mode 1
        TIM17->CCMR1 |= TIM_CCMR1_OC1PE;
        TIM17->CCER |= TIM_CCER_CC1E; // compare 1 output enable

        // TIM17->CCER |= TIM_CCER_CC1P; // invert the output due to the hardware inverter

        TIM17->DIER |= TIM_DIER_CC1DE; // CC1 DMA

        TIM17->CR1 |= TIM_CR1_CEN;
    }

    void setup_dma()
    {

        BOOL_GATE;

        /* The following example is given for the ADC. It can be easily ported on
         any peripheral supporting DMA transfer taking of the associated channel
         to the peripheral, this must check in the datasheet. */
        /* (1) Enable the peripheral clock on DMA */
        /* (2) Enable DMA transfer on ADC */
        /* (3) Configure the peripheral data register address */
        /* (4) Configure the memory address */
        /* (5) Configure the number of DMA tranfer to be performs on channel 1 */
        /* (6) Configure increment, size and interrupts */
        /* (7) Enable DMA Channel 1 */

        // enable the DMA
        RCC->AHBENR |= RCC_AHBENR_DMA1EN; /* (1) */

        // ADC1->CFGR1 |= ADC_CFGR1_DMAEN;                                                                       /* (2) */

        // Destination register
        DMA1_Channel1->CPAR = (uint32_t)(&(TIM17->CCR1)); /* (3) */

        // source address
        DMA1_Channel1->CMAR = (uint32_t)(led_dma_timing_buffer); /* (4) */

        DMA1_Channel1->CNDTR = 25; /* (5) */

        /* (6) */
        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_MINC); // memory increment
        // DO NOT set DMA_CCR_PINC, we want the destination addr to stay the same

        CLEAR_BIT(DMA1_Channel1->CCR, 0b11 << DMA_CCR_MSIZE_Pos); // source size, 00 = 8 bit

        CLEAR_BIT(DMA1_Channel1->CCR, 0b11 << DMA_CCR_PSIZE_Pos); // clear to 0
        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_PSIZE_0);             // destination size 16 bit

        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_TEIE); // enable error interrupt
        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_TCIE); // enable transfer complete interrupt

        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_DIR); // Direction "read from memory" to peripheral

        // enable the DMA
        // DMA1_Channel1->CCR |= DMA_CCR_EN; /* (7) */ // do that later

        /* Configure NVIC for DMA */
        /* (1) Enable Interrupt on DMA Channel 1 */
        /* (2) Set priority for DMA Channel 1 */
        NVIC_EnableIRQ(DMA1_Channel1_IRQn);      /* (1) */
        NVIC_SetPriority(DMA1_Channel1_IRQn, 0); /* (2) */
    }

    void test()
    {
        printf("WS2815 test\n");
        setup_PWM();
        setup_dma();

        PRINT_REG(DMA1_Channel1->CCR);

        memset(led_dma_timing_buffer, 9, 24);

        /**
         * Period = 1500ns
         *
         * 0 Code:
         * CCR1 = 2
         * - 280ns
         * - 1220 ns
         *
         * 1 Code:
         * CCR1 = 9
         * - 1150 ns high
         * - 350 ns low
         *
         */

        printf_arrln("%X", led_dma_timing_buffer, 25);

        DMA1_Channel1->CNDTR = 25;
        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_EN);

        return;
        while (true)
        {
            DMA1_Channel1->CNDTR = 25;
            SET_BIT(DMA1_Channel1->CCR, DMA_CCR_EN);

            while (READ_BIT(DMA1_Channel1->CCR, DMA_CCR_EN))
            {
                ;
            }

            util::delay_ms(1);
        }
    }

#ifdef __cplusplus
    extern "C"
    {
#endif
        void DMA1_Channel1_IRQHandler(void)
        {
            // clear the interrupt bits:

            if (READ_BIT(DMA1->ISR, DMA_ISR_TCIF1))
            {
                CLEAR_BIT(DMA1_Channel1->CCR, DMA_CCR_EN);
            }
            else
            {
                PRINT_REG(DMA1->ISR);
            }
            SET_BIT(DMA1->IFCR, DMA_IFCR_CGIF1);
        }
#ifdef __cplusplus
    }
#endif

}