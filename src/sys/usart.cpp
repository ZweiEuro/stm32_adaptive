#include "sys/usart.hpp"
#include "util.hpp"

#include <cstring>

#include "util.hpp"
#include <queue>

namespace usart
{

    std::queue<uint8_t> input_queue;

#define OVER8 0

    void init(uint32_t baudrate)
    {
        BOOL_GATE;

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

#ifdef __cplusplus
    extern "C"
    {
#endif

        void flush_input()
        {

            while (char_available())
            {
                _getchar();
            }
        }

        bool char_available(void)
        {
            return input_queue.empty() == false;
        }

        unsigned char _getchar(void)
        {

            while (input_queue.empty())
            {
                util::delay_ms(10);
            }

            auto val = input_queue.front();
            input_queue.pop();
            return val;
        }

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
                input_queue.push(USART1->RDR);
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

}