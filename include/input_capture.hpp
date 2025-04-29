#pragma once
#include <inttypes.h>
#include <string.h>

namespace ic
{
    void init_ic();

    int process_signals();
    void disable_ic(void);
    void enable_ic(void);

    uint64_t get_last_time_interrupted();

}
