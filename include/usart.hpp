#pragma once

// ---------------------------------------------------------------------------
#include "stm32f0xx.h"
// ---------------------------------------------------------------------------
#define USART_BUFFER_SIZE 64

/* NVIC Global Priority */
#ifndef USART_NVIC_PRIORITY
#define USART_NVIC_PRIORITY 0x06
#endif
// ---------------------------------------------------------------------------
// Internal USART struct
typedef struct
{
    uint16_t Num;
    uint16_t In;
    uint16_t Out;
    uint16_t Size;
    uint8_t *Buffer;
    uint8_t Initialized;
} USART_t;
// ---------------------------------------------------------------------------

void USART_Init(uint32_t baudrate);
void send_bytes(uint8_t *DataArray, uint16_t count);
void send_bytes_as_hex(uint8_t *DataArray, uint16_t count, char separator);
uint8_t USART_GetChar(void);
uint16_t USART_GetString(char *buffer, uint16_t bufsize);
uint8_t USART_FindCharacter(uint8_t c);

// overloaded senders
void send(const char v);
void send(const char *v);

void send(const int v);
void send(const float v);

// ---------------------------------------------------------------------------
