#pragma once

namespace ws2815
{

    class Color
    {
    public:
        uint8_t _color[3] = {0, 0, 0};

        Color(uint8_t r, uint8_t g, uint8_t b)
        {
            _color[0] = r;
            _color[1] = g;
            _color[2] = b;
        }
        Color() {}

        void print() const
        {
            printf("0x%X 0x%X 0x%X", _color[0], _color[1], _color[2]);
        }

        uint8_t r() const { return _color[0]; }
        uint8_t g() const { return _color[1]; }
        uint8_t b() const { return _color[2]; }
    };

    /**
     * On 8 Mhz;
     * TIM->ARR = 12-1
     * Period = 1500ns
     * 1 'part' = = 1.5us / 12 = 125ns
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
     * Problem: period and switch time rival DMA transfer speeds, very bad idea
     *
     * --------
     *
     * On 32Mhz:
     * TIM->ARR = 48-1
     * Period = 1500ns
     * 1 'part' = = 1.5us / 48 = 312.5ps
     *
     * 0 Code:
     * CCR1 = 8  -> high time = 250  ns
     *
     * 1 Code:
     * CCR1 = 36 -> high time = 1125 ns
     *
     */

    const uint8_t CODE_0_CCR = 2 * (F_CPU / 8000000);
    const uint8_t CODE_1_CCR = 9 * (F_CPU / 8000000);

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

    // pin that produces the pwm signal
#define PIN_PA7_Pos (1 << 7);

#define LED_MAX_COUNT 1

    // control from the outside
    void test();

    void fade_to_color(const Color &c, const uint32_t fade_time = 1000);
}