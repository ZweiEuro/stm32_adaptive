#pragma once
#include "usart.hpp"
#include <inttypes.h>
#include <string.h>

#define IC_DEBUG false
namespace ic
{
    void init_ic();

    int process_signals();
    void disable_ic(void);
    void enable_ic(void);

    uint64_t get_last_time_interrupted();

#define PATTERN_MAX_N 8

    class PeriodPattern
    {
    private:
        int _length;
        float _tolerance;

    public:
        uint16_t periods[PATTERN_MAX_N] = {0};

        int getLength()
        {
            return _length;
        }

        PeriodPattern(const uint16_t signal_pattern[PATTERN_MAX_N], float tolerance = 0.0);

        bool match_window(const uint16_t signal_pattern[PATTERN_MAX_N]);

        void print();
    };
}
