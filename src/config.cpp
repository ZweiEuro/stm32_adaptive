#include "input_capture.hpp"
#include "usart.hpp"

#include "config.hpp"

#include "stdlib.h"

namespace conf
{
    int n_patterns = 0;
    ic::PeriodPattern **period_patterns = nullptr;

    void add_pattern(ic::PeriodPattern *pattern)
    {

        if (conf::period_patterns == nullptr)
        {
            conf::period_patterns = (ic::PeriodPattern **)calloc(sizeof(ic::PeriodPattern *), conf::n_patterns);
        }
        static int index = 0;

        if (index >= conf::n_patterns)
        {
            send("[ERR] patterns full");
        }

        conf::period_patterns[index] = pattern;
        index++;
    }

    void handle_usart(void)
    {
        uint8_t byte = USART_GetByte();
        if (byte == 0)
        {
            return;
        }

        switch (byte)
        {
        case C_SETUP:
            n_patterns = USART_GetByte(true);

            for (int i = 0; i < n_patterns; i++)
            {
                // temporary timing storage
                uint16_t tmp[PATTERN_MAX_N] = {0};
                float tolerance = 0.0;

                for (int period_i = 0; period_i < PATTERN_MAX_N; period_i++)
                {
                    // get the two parts of the number
                    uint8_t time_parts[2] = {0};
                    time_parts[1] = USART_GetByte(true); // the first part is the UPPER value
                    time_parts[0] = USART_GetByte(true);

                    // re-interpret the two uint8_t as a single uint16_t
                    tmp[period_i] = *((uint16_t *)time_parts);
                }

                float t_frac = USART_GetByte(true); // value between 1 - 255 representing fraction of percentage
                tolerance = t_frac / 255.0;

                add_pattern(new ic::PeriodPattern(tmp, tolerance));
            }

            break;

        case C_PRINT:
        case 'p':
        {
            send("print config: n = ");
            send(n_patterns);

            for (int i = 0; i < n_patterns; i++)
            {
                period_patterns[i]->print();
            }

            send("\n");
        }
        break;

        default:
            break;
        }
    }
}