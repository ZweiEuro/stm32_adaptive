#pragma once
#include "usart.hpp"
#include <inttypes.h>
#include <string.h>

#define IC_DEBUG false
namespace ic
{

    int process_signals();

    class PeriodPattern
    {
    private:
        int _length = 8;
        float _tolerance;

    public:
        const uint32_t pattern_length = 8;
        uint16_t periods[8] = {0};

        int getLength()
        {
            return _length;
        }

        PeriodPattern(const uint16_t signal_pattern[8], float tolerance = 0.0)
        {
            _length = 8;
            _tolerance = tolerance;
            memcpy(periods, signal_pattern, 8 * sizeof(uint16_t));

            for (int i = 0; i < 8; i++)
            {
                if (periods[i] == 0)
                {
                    _length = i + 1;
                    break;
                }
            }
        }

        bool match_window(const uint16_t signal_pattern[8])
        {

            if (pattern_length == 0)
            {
                return false;
            }

            for (int index = 0; index < pattern_length; index++)
            {
                const volatile auto target_val = periods[index];
                const volatile auto signal_period = signal_pattern[index];

                if (target_val == 0.0)
                {
                    // we are done
                    return true;
                }

                if (signal_period == 0.0)
                {
                    return false;
                }

                const uint32_t lower_bound = target_val * (1.0 - _tolerance);
                const uint32_t upper_bound = target_val * (1.0 + _tolerance);

                const bool within_tolerance = lower_bound <= signal_period &&
                                              signal_period <= upper_bound;

                if (!within_tolerance)
                {
#if IC_DEBUG
                    send("miss: ");
                    send(index);
                    send(" ");
                    send(lower_bound);
                    send(" <= ");
                    send(signal_period);
                    send(" <= ");
                    send(upper_bound);
                    send('\n');
#endif
                    return false;
                }
                else
                {
#if IC_DEBUG
                    send("good: ");
                    send(index);
                    send(" ");
                    send(lower_bound);
                    send(" <= ");
                    send(signal_period);
                    send(" <= ");
                    send(upper_bound);
                    send('\n');
#endif
                }
            }
            return true;
        }
    };

    void init_ic();

    void bla();
}
