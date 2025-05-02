
#include "sys/printf_getchar.hpp"

#include <cstring>
#include "util.hpp"

#include "features/ws28xx.hpp"

namespace ws2815
{

#define WAIT_LED_STRIP_IDLE         \
    {                               \
        while (strip_state != IDLE) \
        {                           \
            util::delay_ms(1);      \
        }                           \
    }

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

    const uint8_t CODE_0_CCR = 2;
    const uint8_t CODE_1_CCR = 9;

    uint8_t dma_buffer_single_color[24 + 1] = {0};

    const auto dma_values_per_led = sizeof(dma_buffer_single_color);
    const auto values_per_led = sizeof(dma_buffer_single_color) - 1;

    const uint8_t led_dma_timing_buffer_OFF[24 + 1] = {
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,

        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,

        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        CODE_0_CCR,
        0};

    const uint8_t led_dma_timing_buffer_WHITE[24 + 1] = {
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,

        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,

        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        CODE_1_CCR,
        0};

    const auto PIN_PA7_Pos = (1 << 7);

    inline void setup_PWM()
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

    inline void setup_dma()
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
        DMA1_Channel1->CMAR = (uint32_t)(dma_buffer_single_color); /* (4) */

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

    void init()
    {
        setup_PWM();
        setup_dma();
    }

    enum ws2811_state
    {
        IDLE,
        SET_TO_SINGLE_COLOR,
    };

    const uint8_t LED_MAX_COUNT = 100;

    volatile ws2811_state strip_state = IDLE;
    volatile bool START = false;

#ifdef __cplusplus
    extern "C"
    {
#endif

        void DMA1_Channel1_IRQHandler(void)
        {
            // clear the interrupt bits:

            if (READ_BIT(DMA1->ISR, DMA_ISR_TCIF1) || START == true)
            {
                SET_BIT(DMA1->IFCR, DMA_IFCR_CGIF1); // clear status bit
                static volatile uint8_t led_index = 0;

                // reset the timer unit
                CLEAR_BIT(TIM17->CR1, TIM_CR1_CEN);
                TIM17->CNT = 0;

                if (START)
                {
                    printf("starting\n");
                    led_index = 0;
                    START = false;
                }
                else
                {
                    // when starting, the 0th one needs to still be sent
                    led_index++;
                }

                CLEAR_BIT(DMA1_Channel1->CCR, DMA_CCR_EN); // disable so we can change settings
                DMA1_Channel1->CNDTR = dma_values_per_led; // ready for next LED information

                if (led_index >= LED_MAX_COUNT)
                {
                    printf("strip done\n");
                    strip_state = IDLE;
                    return; // we are done!
                }

                switch (strip_state)
                {
                case SET_TO_SINGLE_COLOR:
                    DMA1_Channel1->CMAR = (uint32_t)(dma_buffer_single_color);
                    break;

                default:
                    break;
                }

                SET_BIT(TIM17->CR1, TIM_CR1_CEN);
                SET_BIT(DMA1_Channel1->CCR, DMA_CCR_EN);
            }
            else
            {
                PRINT_REG(DMA1->ISR);
            }
        }
#ifdef __cplusplus
    }
#endif

    void strip_do(ws2811_state new_state)
    {

        // check if idle, wait if not
        if (strip_state != IDLE)
        {
            printf("[Err] Strip busy");

            WAIT_LED_STRIP_IDLE;
        }

        // switch according to what state we want to be in now

        strip_state = new_state;

        START = true;

        DMA1_Channel1_IRQHandler();
        WAIT_LED_STRIP_IDLE;
    }

    void test()
    {
        printf("WS2815 test\n");

        static int counter = 0;

        if (counter % 2 == 0)
        {

            printf("off\n");

            memset(dma_buffer_single_color, CODE_0_CCR, values_per_led);

            strip_do(SET_TO_SINGLE_COLOR);
        }

        if (counter % 2 == 1)
        {
            printf("on\n");
            memset(dma_buffer_single_color, CODE_1_CCR, values_per_led);
            strip_do(SET_TO_SINGLE_COLOR);
        }

        counter++;
    }

}