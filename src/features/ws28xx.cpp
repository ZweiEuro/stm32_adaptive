
#include "sys/printf_getchar.hpp"

#include <cstring>
#include "util.hpp"

#include "features/ws28xx.hpp"
#include "features/math.hpp"

using namespace math;

namespace ws2815
{

    Color::Color(uint8_t r, uint8_t g, uint8_t b)
    {
        _color[0] = r;
        _color[1] = g;
        _color[2] = b;
    }

    Color Color::lerp(const Color &a, const Color &b, const int percent)
    {
        return Color(
            math::lerp(a.r(), b.r(), percent),
            math::lerp(a.g(), b.g(), percent),
            math::lerp(a.b(), b.b(), percent));
    }

    WS2815 ws2815;

    void WS2815::set_dma_timings_for_color(const Color &color)
    {

        // color needs to be send out with G R B
        // HIGH bit first!

        _current_color_all_leds = color;

        uint8_t grb[] = {
            _current_color_all_leds.g(),
            _current_color_all_leds.r(),
            _current_color_all_leds.b()};

        auto target = selected_buffer_1 ? _current_color_dma_buffer_2 : _current_color_dma_buffer_1;

        for (int byte = 0; byte < 3; byte++)
        {
            for (int i = 0; i < 8; i++)
            {
                target[(7 - i) + 8 * byte] = grb[byte] & (1 << i) ? CODE_1_CCR : CODE_0_CCR;
            }
        }
        new_color = true;
    }

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
        DMA1_Channel1->CMAR = (uint32_t)(&ZERO); // garbage pointer /* (4) */

        DMA1_Channel1->CNDTR = DMA_TRANSFERS_RES_SIG; /* (5) */

        /* (6) */
        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_MINC); // memory increment
        // DO NOT set DMA_CCR_PINC, we want the destination addr to stay the same

        CLEAR_BIT(DMA1_Channel1->CCR, 0b11 << DMA_CCR_MSIZE_Pos); // source size, 00 = 8 bit

        CLEAR_BIT(DMA1_Channel1->CCR, 0b11 << DMA_CCR_PSIZE_Pos); // clear to 0
        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_PSIZE_0);             // destination size 16 bit

        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_TEIE); // enable error interrupt
        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_TCIE); // enable transfer complete interrupt

        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_DIR); // Direction "read from memory" to peripheral

        /* Configure NVIC for DMA */
        /* (1) Enable Interrupt on DMA Channel 1 */
        /* (2) Set priority for DMA Channel 1 */
        NVIC_EnableIRQ(DMA1_Channel1_IRQn);      /* (1) */
        NVIC_SetPriority(DMA1_Channel1_IRQn, 0); /* (2) */

        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_CIRC); // make the memory loop

        // enable the DMA
        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_EN);
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

            if (READ_BIT(DMA_ISR, DMA_ISR_TCIF1))
            {
                static auto _led_index = 0;

                if (_led_index >= LED_INDEX_RESET_SIGNAL)
                {
                    // if reset signal to be send or done sending
                    CLEAR_BIT(DMA1_Channel1->CCR, DMA_CCR_EN); // disable so we can change settings

                    if (_led_index == LED_INDEX_RESET_SIGNAL)
                    {
                        // always send RESET after full set of data has been sent
                        CLEAR_BIT(DMA1_Channel1->CCR, DMA_CCR_MINC);
                        DMA1_Channel1->CNDTR = DMA_TRANSFERS_RES_SIG;
                        DMA1_Channel1->CMAR = (uint32_t)(&ZERO);
                    }
                    else if (_led_index >= LED_INDEX_DONE)
                    {
                        util::toggle_onboard();

                        _led_index = 0;

                        // this is always AFTER the reset has been sent, we can do more logic within the switch
                        // but its important we turn on memory incrementing again!
                        SET_BIT(DMA1_Channel1->CCR, DMA_CCR_MINC);

                        DMA1_Channel1->CNDTR = DMA_TRANSFERS_PER_LED;
                        DMA1_Channel1->CMAR = (uint32_t)(ws2815.get_dma_pointer());
                    }
                    SET_BIT(DMA1_Channel1->CCR, DMA_CCR_EN);
                }
                // run conditions

                _led_index++;
            }
        }
#ifdef __cplusplus
    }
#endif

    void test()
    {
        printf("WS2815 test\n");
        static int counter = 0;

        const auto r = Color{0xFF, 0x00, 0x00};
        const auto g = Color{0x00, 0xFF, 0x00};
        const auto b = Color{0x00, 0x00, 0xFF};
        const auto lila = Color::lerp(b, r, 512);

        switch (counter % 4)
        {
        case 0:
        {
            ws2815.fade_between_colors(r, g, 500);
            //  ws2815.to_color(c);
            break;
        }
        case 1:
        {
            ws2815.fade_to_color(g);
            //  ws2815.to_color(c);
            break;
        }

        case 2:
        {
            ws2815.fade_to_color(b);
            //  ws2815.to_color(c);
            break;
        }

        case 3:
        {
            ws2815.fade_to_color(lila);
            //  ws2815.to_color(c);
            break;
        }
        }

        counter++;
    }

    void WS2815::process()
    {
        switch (current_cmd)
        {
        case _commands::IDLE:
            return;
        case _commands::TO_COLOR:
        {
            return;
        }
        break;
        case _commands::FADE_BETWEEN_COLORS:
        case _commands::FADE_TO_COLOR:
        {

            const uint64_t ms_since_start_shifted = (rcc::getSystick() - _command_start_systick) << 10;
            const uint64_t percent = (ms_since_start_shifted / fade_time);

            if (percent >= 1025)
            {

                if (current_cmd == FADE_BETWEEN_COLORS)
                {
                    ws2815._command_start_systick = rcc::getSystick();

                    if (current_color_a)
                    {
                        fade_start_color = fade_between_color_a;
                        fade_target_color = fade_between_color_b;
                    }
                    else
                    {
                        fade_start_color = fade_between_color_b;
                        fade_target_color = fade_between_color_a;
                    }

                    current_color_a = !current_color_a;
                    return;
                }

                current_cmd = _commands::IDLE;
                return;
            }

            auto next_color = Color(
                math::lerp(fade_start_color.r(), fade_target_color.r(), percent),
                math::lerp(fade_start_color.g(), fade_target_color.g(), percent),
                math::lerp(fade_start_color.b(), fade_target_color.b(), percent));

            set_dma_timings_for_color(next_color);
        }
        break;

        default:
            printf("[Err] cannot process state for ws2815!\n");
            break;
        }
    }

    void WS2815::to_color(const Color &c)
    {

        ws2815.set_dma_timings_for_color(c);
        current_cmd = _commands::TO_COLOR;
    }

    void WS2815::fade_between_colors(const Color &a, const Color &b, const uint32_t fade_time)
    {
        ws2815.fade_between_color_a = a;
        ws2815.fade_between_color_b = b;
        ws2815._command_start_systick = rcc::getSystick();

        ws2815.fade_start_color = ws2815._current_color_all_leds;
        ws2815.fade_time = fade_time;
        fade_target_color = a;
        current_color_a = true;

        current_cmd = _commands::FADE_BETWEEN_COLORS;
    }

    void WS2815::fade_to_color(const Color &c, const uint32_t fade_time)
    {

        ws2815.fade_time = fade_time;
        ws2815.fade_start_color = ws2815._current_color_all_leds;
        ws2815.fade_target_color = c;
        ws2815._command_start_systick = rcc::getSystick();
        current_cmd = _commands::FADE_TO_COLOR;
    }
}