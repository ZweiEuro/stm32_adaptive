#include "stm32f030x6.h"

#include "usart.h"
#include "rcc.h"

#define LEDPORT (GPIOA)
#define LED1 (4)
#define ENABLE_GPIO_CLOCK (RCC->AHBENR |= RCC_AHBENR_GPIOAEN)
#define GPIOMODER (GPIO_MODER_MODER4_0)

void ms_delay(int ms)
{
  while (ms-- > 0)
  {
    volatile int x = 500;
    while (x-- > 0)
      __asm("nop");
  }
}

// Alternates blue and green LEDs quickly
int main(void)
{

  ENABLE_GPIO_CLOCK;           // enable the clock to GPIO
  LEDPORT->MODER |= GPIOMODER; // set pins to be general purpose output

  char buffer[64];
  RCC_Init();
  USART_Init(9600);

  USART_SendString("hello world!\n");

  while (1)
  {

    /* Get string */
    if (USART_GetString(buffer, sizeof(buffer)))
    {
      /* Return string back */
      USART_SendString(buffer);
    }
  }

  return 0;
}