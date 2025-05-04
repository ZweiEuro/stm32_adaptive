#pragma once

namespace ws2815
{

    class Color
    {
    public:
        uint8_t r, g, b;

    public:
        Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
        Color() : r(0), g(0), b(0) {}

        void print() const
        {
            printf("%X %X %X\n", r, g, b);
        }
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

#define LED_MAX_COUNT 10

    class WS2815
    {
    public:
        enum _states
        {
            IDLE,
            ABORTING,

            // Functionality
            TO_COLOR,
        };

    public: // public so the ISR can see them
        _states _current_state = _states::IDLE;
        _states _state_buffer = _states::IDLE;

        uint8_t _dma_buffer_all_leds[24 + 1] = {0};
        Color _current_color_all_leds = Color(0, 0, 0);

        // command start systick
        uint64_t _command_start_systick = 0;
        int _led_index = 0;

    public:
        uint32_t fade_time = 1000; // color fade time
        Color fade_target_color;

        void to_state(_states new_state);

        WS2815();
    };

    void test();

}