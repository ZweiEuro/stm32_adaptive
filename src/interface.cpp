#include "input_capture.hpp"

#include "interface.hpp"

#include "stdlib.h"
#include "main.hpp"
#include "sender.hpp"
#include "storage/flash.hpp"
#include "sys/printf_getchar.hpp"

#include "features/ws28xx.hpp"

namespace interface
{

    std::queue<uint8_t> found_indices;

    void handle_usart(void)
    {
        if (char_available() == false)
        {
            return;
        }

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
                tmp[i]._tolerance_mul_255 = getchar();
            }

            flash::savePatterns(tmp, n_patterns);
            free(tmp);
        }
        break;

        case C_START:
            ic::enable_ic();
            printf("ic enable");
            break;
        case C_HALT:
            ic::disable_ic();
            printf("ic disable");
            break;
        case C_FLUSH:
            printf("found indices:\n");
            while (found_indices.empty() == false)
            {
                printf("%d ", found_indices.front());
                found_indices.pop();
            }
            printf("\n");

            break;

        case C_DEV_TEST: // 'a'
            ws2815::test();
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