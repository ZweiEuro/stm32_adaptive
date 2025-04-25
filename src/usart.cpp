#include "usart.hpp"
#include "util.hpp"

#include <cstring>

/* Set variables for buffers */
typedef struct
{
    uint8_t buffer[USART_BUFFER_SIZE];
    bool occupied[USART_BUFFER_SIZE];

    int read_head = 0;
    int write_head = 0;
} USART;

USART usart;

#define OVER8 0

void USART_Init(uint32_t baudrate)
{

    memset(usart.occupied, false, sizeof(usart.occupied));

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

bool USART_GetByte(uint8_t &dest, bool blocking)
{

    if (blocking)
    {

        while (usart.occupied[usart.read_head] == false)
        {
            util::delay_ms(10);
        }
    }
    else
    {
        // if its not occupied, we cant return anything
        if (usart.occupied[usart.read_head] == false)
        {
            return false;
        }
    }

    // we made sure there is data there that we want
    dest = usart.buffer[usart.read_head];
    usart.occupied[usart.read_head] = false;
    usart.read_head = (usart.read_head + 1) % USART_BUFFER_SIZE;
    return true;
}

#ifdef __cplusplus
extern "C"
{
#endif

    void _putchar(const char c)
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

    void USART1_IRQHandler(void)
    {

        /* Check if interrupt was because data is received */
        if (USART1->ISR & USART_ISR_RXNE)
        {

            if (usart.occupied[usart.write_head] == false)
            {
                usart.buffer[usart.write_head] = USART1->RDR;
                usart.occupied[usart.write_head] = true;
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