#include "input_capture.hpp"

#include "interface.hpp"

#include "stdlib.h"
#include "main.hpp"
#include "sender.hpp"
#include "flash.hpp"
#include "sys/printf_getchar.hpp"

namespace interface
{
    int n_patterns = 0;
    ic::PeriodPattern **period_patterns = nullptr;

    void add_pattern(ic::PeriodPattern *pattern)
    {

        if (interface::period_patterns == nullptr)
        {
            interface::period_patterns = (ic::PeriodPattern **)calloc(sizeof(ic::PeriodPattern *), interface::n_patterns);
        }
        static int index = 0;

        if (index >= interface::n_patterns)
        {
            printf("[ERR] patterns full");
        }

        interface::period_patterns[index] = pattern;
        index++;
    }

    void handle_usart(void)
    {

        switch (getchar())
        {
        case C_SETUP:

            n_patterns = getchar();

            for (int i = 0; i < n_patterns; i++)
            {
                // temporary timing storage
                uint16_t tmp[PATTERN_MAX_N] = {0};
                float tolerance = 0.0;

                for (int period_i = 0; period_i < PATTERN_MAX_N; period_i++)
                {
                    // get the two parts of the number
                    uint8_t time_parts[2] = {0};
                    time_parts[1] = getchar(); // the first part is the UPPER value
                    time_parts[0] = getchar();
                    // re-interpret the two uint8_t as a single uint16_t
                    tmp[period_i] = *((uint16_t *)time_parts);
                }

                // tolerance value value between 1 - 255 representing fraction of percentage
                add_pattern(new ic::PeriodPattern(tmp, getchar()));
            }

            break;

        case C_START:
            ic::enable_ic();
            printf("s");
            break;
        case C_HALT:
            ic::disable_ic();
            printf("h");
            break;
        case C_FLUSH:
            prinf_arrln("%ld", global::found_signals, sizeof(global::found_signals));

            break;

        case C_FLASH_TEST:
            flash::test();
            break;

        case C_PRINT:
        {
#if 0
            send("print config: n = ");
            send(n_patterns);
            send('\n');

            for (int i = 0; i < n_patterns; i++)
            {
                period_patterns[i]->print();
            }

            send("\n");
#endif
        }
        break;

        case C_TEST:
        {
            static bool toggle = false;

            printf("sending\n");

            if (toggle)
            {
                const uint8_t arr[] = {
                    0,
                    1,
                    2,
                    1,
                    1,
                    1,
                    1,
                    1,
                    1,
                    1,
                    2,
                    2,
                    2,
                    0,
                };
                sender::send_434(arr, sizeof(arr));
            }
            else
            {
                const uint8_t arr[] = {
                    0,
                    1,
                    2,
                    1,
                    1,
                    1,
                    1,
                    1,
                    1,
                    1,
                    2,
                    2,
                    1,
                    0,
                };
                sender::send_434(arr, sizeof(arr));
            }

            toggle != toggle;
        }
        break;

        default:
            break;
        }
    }
}