
#include "sys/printf_getchar.hpp"

#include <cstring>
#include "util.hpp"

#include "features/ws28xx.hpp"
#include "features/math.hpp"

using namespace math;

namespace ws2815
{

#define WAIT_LED_STRIP_IDLE         \
    {                               \
        while (strip_state != IDLE) \
        {                           \
            util::delay_ms(1);      \
        }                           \
    }

    static WS2815 ws2815;

    void set_dma_timings_for_color(uint8_t *dma_buffer, const Color color)
    {
        // color needs to be send out with G R B

        for (int i = 0; i < 8; i++)
        {
            if (color.g & (1 << i))
            {
                dma_buffer[i] = CODE_1_CCR;
            }
            else
            {
                dma_buffer[i] = CODE_0_CCR;
            }
        }

        for (int i = 0; i < 8; i++)
        {
            if (color.r & (1 << i))
            {
                dma_buffer[i + 8] = CODE_1_CCR;
            }
            else
            {
                dma_buffer[i + 8] = CODE_0_CCR;
            }
        }

        for (int i = 0; i < 8; i++)
        {
            if (color.b & (1 << i))
            {
                dma_buffer[i + 16] = CODE_1_CCR;
            }
            else
            {
                dma_buffer[i + 16] = CODE_0_CCR;
            }
        }
    }

    const auto DMA_TRANSFERS_PER_LED = 25;
    const auto DMA_BIT_VALUES_PER_LED = 24;

    inline void setup_PWM()
    {

        BOOL_GATE;

        // PIN PA7
        SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOAEN); // Enable GPIOA bank

        // PA7 into alternate function mode
        GPIOA->MODER |= (0b10 << GPIO_MODER_MODER7_Pos);
        GPIOA->OSPEEDR |= (0b11 << GPIO_MODER_MODER7_Pos); // high speed

        GPIOA->ODR |= PIN_PA7_Pos;

        // PA7 into alternate function 5 -> TIM17_CH1
        SET_BIT(GPIOA->AFR[0], 0b0101 << GPIO_AFRL_AFSEL7_Pos);

        // enable the timer, TIM17_CH1
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM17EN);

        //// no prescaler, 72MHz clock
        TIM17->PSC = 0;
        TIM17->ARR = (12 * (F_CPU / 8000000)) - 1;

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
        DMA1_Channel1->CMAR = (uint32_t)(0); // garbage pointer /* (4) */

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

    WS2815::WS2815()
    {
        setup_PWM();
        setup_dma();
    }

#ifdef __cplusplus
    extern "C"
    {
#endif

        void DMA1_Channel1_IRQHandler(void)
        {

            const auto DMA_ISR = DMA1->ISR;
            SET_BIT(DMA1->IFCR, DMA_IFCR_CGIF1); // clear status bit

            if (READ_BIT(DMA_ISR, DMA_ISR_TEIF1))
            {
                printf("[Err] DMA transfer error!\n");
                return;
            }

            // clear the interrupt bits:

            if (READ_BIT(DMA_ISR, DMA_ISR_TCIF1) || ws2815._current_state != WS2815::IDLE)
            {
                if (ws2815._current_state == WS2815::ABORTING)
                {
                    ws2815._current_state = ws2815._state_buffer;
                    ws2815._state_buffer = WS2815::IDLE;
                    ws2815._led_index = 0;
                }

                util::toggle_onboard();

                CLEAR_BIT(DMA1_Channel1->CCR, DMA_CCR_EN);    // disable so we can change settings
                DMA1_Channel1->CNDTR = DMA_TRANSFERS_PER_LED; // ready for next LED information

                // check abort conditions

                // run conditions
                switch (ws2815._current_state)
                {
                case WS2815::_states::TO_COLOR:
                    if (ws2815._led_index == 0)
                    {

                        DMA1_Channel1->CMAR = (uint32_t)(ws2815._dma_buffer_all_leds);

                        set_dma_timings_for_color(ws2815._dma_buffer_all_leds, ws2815.fade_target_color);
                    }
                    else if (ws2815._led_index >= LED_MAX_COUNT)
                    {
                        ws2815._current_state = WS2815::IDLE;
                        ws2815._current_color_all_leds = ws2815.fade_target_color;
                        printf("Done \n");
                        return;
                    }

                    break;

                default:
                    break;
                }

                ws2815._led_index++;
                SET_BIT(DMA1_Channel1->CCR, DMA_CCR_EN);
            }
        }
#ifdef __cplusplus
    }
#endif

    void test()
    {
        printf("WS2815 test\n");

        static int counter = 0;

        if (counter % 2 == 0)
        {
            ws2815.fade_target_color = Color(0xFF, 0, 0);
        }

        if (counter % 2 == 1)
        {
            ws2815.fade_target_color = Color{0, 0, 0x00};
        }

        ws2815.to_state(WS2815::TO_COLOR);
        counter++;
    }

    void WS2815::to_state(WS2815::_states new_state)
    {
        _command_start_systick = rcc::getSystick();

        _led_index = 0;

        if (_current_state == IDLE)
        {
            printf("Started new command \n");
            // just do it and give it a kick
            _current_state = new_state;
            DMA1_Channel1_IRQHandler();
        }
        else
        {
            printf("command interrupted!\n");

            _state_buffer = new_state;
            _current_state = ABORTING;
            // it should start up as soon as it can
        }
    }
}