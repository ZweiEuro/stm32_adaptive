#pragma once

#include "stm32f0xx.h"
#include "stdint.h"

#define USART_BUFFER_SIZE 64

/* NVIC Global Priority */
#ifndef USART_NVIC_PRIORITY
#define USART_NVIC_PRIORITY 0x06
#endif

namespace usart
{

    void init(uint32_t baudrate);

#ifdef __cplusplus
    extern "C"
    {
#endif

        void _putchar(const char c);
        unsigned char _getchar(void);

#ifdef __cplusplus
    }
#endif
}