#include "stdint.h"
#include "features/math.hpp"
namespace math
{

    /**
     * percent = 0 - 1024
     */
    int lerp(int from, int to, int percent)
    {
        return (from * (1024 - percent) + to * percent) >> 10;
    }
}