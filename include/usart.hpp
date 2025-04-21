#pragma once

// ---------------------------------------------------------------------------
#include "stm32f0xx.h"
// ---------------------------------------------------------------------------
#define USART_BUFFER_SIZE 64

/* NVIC Global Priority */
#ifndef USART_NVIC_PRIORITY
#define USART_NVIC_PRIORITY 0x06
#endif

void USART_Init(uint32_t baudrate);

// USART
bool USART_GetByte(uint8_t &dest, bool blocking = false);

// overloaded senders
void send(const char v);
void send(const char *v);

void send(const int v);
void send(const uint32_t v);
void send(const float v);

template <typename T>
void send_array(T *array, int size)
{

    send("[");

    for (int i = 0; i < size; i++)
    {

        send(array[i]);
        if (i < (size - 1))
        {
            send(", ");
        }
    }

    send("]");
}

void send_bin(uint32_t v);

// ---------------------------------------------------------------------------
