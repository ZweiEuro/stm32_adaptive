#include "classes/PeriodPattern.hpp"

#include <cstring>
#include "sys/printf_getchar.hpp"

// CLASS

#define IC_DEBUG 0

PeriodPattern::PeriodPattern(const uint16_t signal_pattern[PATTERN_MAX_N], uint8_t tolerance)
{
    _tolerance_mul_255 = tolerance;
    memcpy(periods, signal_pattern, 8 * sizeof(uint16_t));
}

int PeriodPattern::getLength() const
{
    for (int i = 0; i < 8; i++)
    {
        if (periods[i] == 0)
        {
            return i;
            break;
        }
    }
    return PATTERN_MAX_N;
}

bool PeriodPattern::match_window(const uint16_t signal_pattern[PATTERN_MAX_N])
{

    const auto len = getLength();

    if (len == 0)
    {
        return false;
    }

    for (int index = 0; index < len; index++)
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

        // fixed point math to spare ourselves the float struggle
        uint32_t lower_bound = target_val * 255 - target_val * _tolerance_mul_255;
        lower_bound /= 255;
        uint32_t upper_bound = target_val * 255 + target_val * _tolerance_mul_255;
        upper_bound /= 255;

        const bool within_tolerance = lower_bound <= signal_period &&
                                      signal_period <= upper_bound;

        if (!within_tolerance)
        {
#if IC_DEBUG

            printf("miss [%d]: %ld <= %ld <= %ld\n", index, lower_bound, signal_period, upper_bound);

#endif
            return false;
        }
        else
        {
#if IC_DEBUG
            printf("hit [%d]: %ld <= %ld <= %ld\n", index, lower_bound, signal_period, upper_bound);

#endif
        }
    }
    return true;
}

void PeriodPattern::print()
{
    printf("[PP] l: %ld t: %ld/256 timings: ", getLength(), _tolerance_mul_255);
    printf_arrln("%ld", this->periods, this->getLength());
}
