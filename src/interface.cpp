#include "input_capture.hpp"

#include "interface.hpp"

#include "stdlib.h"
#include "main.hpp"
#include "sender.hpp"
#include "storage/flash.hpp"
#include "sys/printf_getchar.hpp"

namespace interface
{

    void handle_usart(void)
    {

        switch (getchar())
        {
        case C_SETUP:
        {
            auto n_patterns = getchar();

            auto tmp = (PeriodPattern *)calloc(sizeof(PeriodPattern), n_patterns);

            for (int i = 0; i < n_patterns; i++)
            {

                for (int period_i = 0; period_i < PATTERN_MAX_N; period_i++)
                {
                    // get the two parts of the number
                    uint8_t time_parts[2] = {0};
                    time_parts[1] = getchar(); // the first part is the UPPER value
                    time_parts[0] = getchar();
                    // re-interpret the two uint8_t as a single uint16_t
                    tmp[i].periods[period_i] = *((uint16_t *)time_parts);
                }

                // tolerance value value between 1 - 255 representing fraction of percentage
                tmp[i]._tolerance_mul_256 = getchar();
            }

            flash::savePatterns(tmp, n_patterns);
            free(tmp);
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
            printf_arrln("%ld", global::found_signals, sizeof(global::found_signals));

            break;

        case C_DEV_TEST: // 'a'

            printf("sizeof: %d", sizeof(PeriodPattern));
            for (int i = 0;; i++)
            {
                auto p = flash::getPattern(i);
                printf("%p\n", p);
                if (p == nullptr)
                    break;
                p->print();
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