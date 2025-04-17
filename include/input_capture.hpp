
#include "usart.hpp"
#include <inttypes.h>
#include <string.h>

namespace ic
{

    template <uint32_t pl>
    class PeriodPattern
    {
    public:
        const uint32_t pattern_length = pl;
        uint16_t periods[pl] = {0};
        float tolerance;

        PeriodPattern(const uint16_t signal_pattern[pl], float tolerance = 0.0)
        {
            this->tolerance = tolerance;
            memcpy(this->periods, signal_pattern, pl * sizeof(uint16_t));
        }

        bool match_window(const uint16_t signal_pattern[pl])
        {

            if (pattern_length == 0)
            {
                return false;
            }

            for (int index = 0; index < pattern_length; index++)
            {
                const auto target_val = periods[index];
                const auto signal_period = signal_pattern[index];

                if (target_val == 0.0)
                {
                    // we are done
                    return true;
                }

                if (signal_period == 0.0 ||
                    !(target_val * (1.0 - tolerance) <= signal_period && signal_period <= target_val * (1.0 + tolerance)))
                {
                    send("missed index: ");
                    send(index);
                    send("\n");
                    return false;
                }
                else
                {
                    send("hit \n");
                    return true;
                }
            }
            return true;
        }
    };

    void init_ic();
}
