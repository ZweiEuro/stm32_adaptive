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

        void println() const
        {
            printf("0x%X 0x%X 0x%X\n", _color[0], _color[1], _color[2]);
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

    const uint8_t led_dma_timing_buffer_OFF[24] = {
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
        CODE_0_CCR};

    const uint8_t led_dma_timing_buffer_WHITE[24] = {
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
        CODE_1_CCR};

    const uint8_t ZERO = 0;

    // pin that produces the pwm signal
#define PIN_PA7_Pos (1 << 7);

#define LED_INDEX_START 0
#define LED_MAX_INDEX 150 // number of LEDs
#define LED_INDEX_RESET_SIGNAL LED_MAX_INDEX
#define LED_INDEX_DONE LED_INDEX_RESET_SIGNAL + 1

    const auto DMA_TRANSFERS_PER_LED = sizeof(led_dma_timing_buffer_WHITE);
    const auto DMA_TRANSFERS_RES_SIG = 180; // roughly estimated since its hard to actually calc with processing time
    const auto DMA_BIT_VALUES_PER_LED = 24;

    // control from the outside
    void test();

    // internal control class
    class WS2815
    {

    public:
        enum _commands
        {
            IDLE,
            // Functionality
            TO_COLOR,
            FADE_TO_COLOR,
        };

        _commands current_cmd = _commands::IDLE;

    private:
        bool new_color = false;
        bool selected_buffer_1 = true;

        // public so the ISR can see them
        // the current DMA that is cycled to the LEDs
        uint8_t _current_color_dma_buffer_1[24] = {0};
        uint8_t _current_color_dma_buffer_2[24] = {0};

    public:
        Color _current_color_all_leds = Color(0, 0, 0); // the current Color representation of the DMA buffer

        // command start systick
        uint64_t _command_start_systick = 0;

    public:
        // FADE_TO_COLOR
        uint64_t fade_time = 1000; // color fade time
        Color fade_start_color;
        Color fade_target_color;

        WS2815();

        // set current color
        void set_dma_timings_for_color(const Color &color);

        // general control
        void process();
        void to_color(const Color &c);
        void fade_to_color(const Color &c, const uint32_t fade_time = 1000);

        bool busy() { return current_cmd != IDLE; }

        inline auto get_dma_pointer()
        {
            if (new_color)
            {
                selected_buffer_1 = !selected_buffer_1;
                new_color = false;
            }

            if (selected_buffer_1)
            {
                return _current_color_dma_buffer_1;
            }
            else
            {
                return _current_color_dma_buffer_2;
            }
        }
    };

    extern WS2815 ws2815;

}