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

    inline bool eat_whitespace()
    {
        if (char_available() == false || getchar() != ' ')
        {
            return false;
        }
        return true;
    }

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

        case C_FADE_TO_COLOR:
        { // scan in "f 0xFF 0xFF 0xFF 10000" to fade to white in 1 second
            // only allowed time values between 00000 and 99999 ( 5 symbols everytime )

            printf("fade subcommand:");
            char command_buffer[sizeof("0xFF 0xFF 0xFF 10000")] = {0};

            // scan in the entire command
            for (auto i = 0; i < sizeof(command_buffer) - 1; i++)
            {
                command_buffer[i] = getchar();

                printf("%c", command_buffer[i]);
                if (command_buffer[i] == 0x8)
                {
                    i = i - 2; // the continue will add 1 back to the total
                    continue;
                }
            }
            printf("\n");
            // printf("entire command:  %s\n", command_buffer);

            ws2815::Color color;

            // get first part
            char *token = strtok(command_buffer, " ");
            color._color[0] = strtol(token, NULL, 0);
            token = strtok(NULL, " ");
            color._color[1] = strtol(token, NULL, 0);
            token = strtok(NULL, " ");
            color._color[2] = strtol(token, NULL, 0);

            // fade time
            token = strtok(NULL, " ");
            auto fade_time = strtol(token, NULL, 10);

            // printf("%d", fade_time);

            ws2815::fade_to_color(color, fade_time);

            return;
        }
        break;

        default:
            break;
        }
    }
}