#pragma once

#include "stm32f0xx.h"
namespace rcc
{

    void RCC_init(void);

    void SYSTICK_init(void);
    uint32_t getSystick();
}