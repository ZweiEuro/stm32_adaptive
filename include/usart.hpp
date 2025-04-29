#pragma once

// ---------------------------------------------------------------------------
#include "stm32f0xx.h"
#include "stdint.h"
// ---------------------------------------------------------------------------
#define USART_BUFFER_SIZE 64

/* NVIC Global Priority */
#ifndef USART_NVIC_PRIORITY
#define USART_NVIC_PRIORITY 0x06
#endif

void USART_Init(uint32_t baudrate);

// USART
bool USART_GetByte(uint8_t &dest, bool blocking = false);

#define send(...)
#define sendln(...)
#define send_array(...)
#define send_arrayln(...)

#ifdef __cplusplus
extern "C"
{
#endif

    void _putchar(const char c);
#ifdef __cplusplus
}
#endif
