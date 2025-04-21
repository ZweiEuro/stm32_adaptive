#include "usart.hpp"
#include <stdio.h>
#include "util.hpp"

/* Set variables for buffers */
typedef struct
{
    uint8_t buffer[USART_BUFFER_SIZE];
    int read_head = 0;
    int write_head = 0;
} USART;

USART usart;

#define OVER8 0

void USART_Init(uint32_t baudrate)
{
    /* Enable GPIOA clock */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    /* Enable USART clock */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    /* PA9 (TX) & PA10 (RX) - Alternate function mode */
    GPIOA->MODER |= (GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1);
    GPIOA->AFR[1] |= (1 << GPIO_AFRH_AFSEL9_Pos) | (1 << GPIO_AFRH_AFSEL10_Pos);
    /* Set USART baudrate */
#if OVER8 == 1
    USART1->BRR = (F_CPU + baudrate / 2) / baudrate;
#else
    USART1->BRR = F_CPU / baudrate;
#endif

    /* Set 8 bit and 1 stop bit */
    USART1->CR1 &= ~(USART_CR1_M | USART_CR2_STOP);
    /* Enable transmitter and receiver USART1 */
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
    /* Enable RX interrupt */
    USART1->CR1 |= USART_CR1_RXNEIE;
    /* Set IRQ priority */
    NVIC_SetPriority(USART1_IRQn, USART_NVIC_PRIORITY);
    /* Enable RX interrupt */
    NVIC_EnableIRQ(USART1_IRQn);
    /* Enable USART peripheral */
    USART1->CR1 |= USART_CR1_UE;
}

uint8_t USART_GetByte(bool blocking)
{
    uint8_t ret = usart.buffer[usart.read_head];

    while (ret == 0 && blocking)
    {
        ret = usart.buffer[usart.read_head];
        util::delay_ms(100);
    }

    if (ret != 0)
    {
        usart.read_head = (usart.read_head + 1) % USART_BUFFER_SIZE;

        // show activity
        GPIOA->ODR ^= (1 << 4);
    }

    return ret;
}

void send(const char c)
{

    /* Check USART if enabled */
    if ((USART1->CR1 & USART_CR1_UE))
    {
        /* Wait to be ready, buffer empty */
        while (!(USART1->ISR & USART_ISR_TC))
            ;
        /* Send data */
        USART1->TDR = (uint16_t)(c);
    }
}

void send(const char *v)
{
    while ((*v) != '\0')
    {
        send(*v);
        v++;
    }
}

void send(const float v)
{

    send((int)(v * 100));
}

void send(const int v)
{
    char buffer[20] = {0};
    int ret = sprintf(buffer, "%d", v);

    if (0 >= ret || ret > (int)sizeof(buffer))
    {
        send("[UART ERR] Could not send int value!\n");
    }
    else
    {
        send(buffer);
    }
}

void send(const uint32_t v)
{
    char buffer[20] = {0};
    int ret = sprintf(buffer, "%lu", v);

    if (0 >= ret || ret > (int)sizeof(buffer))
    {
        send("[UART ERR] Could not send uint32_t value!\n");
    }
    else
    {
        send(buffer);
    }
}

void send_bin(uint32_t v)
{

    send("0b");

    for (int i = 0; i < 32; i++)
    {
        if (v & (1 << (31 - i))) // we walk in reverse
        {
            send("1 ");
        }
        else
        {
            send("0 ");
        }
    }
}

#ifdef __cplusplus
extern "C"
{
#endif

    void USART1_IRQHandler(void)
    {
        GPIOA->ODR ^= (1 << 4);

        /* Check if interrupt was because data is received */
        send_bin(USART1->ISR);
        send('\n');
        if (USART1->ISR & USART_ISR_RXNE)
        {

            if (usart.buffer[usart.write_head] == 0)
            {
                usart.buffer[usart.write_head] = USART1->RDR;
                usart.write_head = (usart.write_head + 1) % USART_BUFFER_SIZE;
            }
        }

        if (USART1->ISR & USART_ISR_ORE)
        {
            // clear the error
            USART1->ICR |= USART_ICR_ORECF;
        }
    }

// Declarations of this file
#ifdef __cplusplus
}
#endif