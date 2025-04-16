#include "stm32f0xx.h"
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
  for (;;)
  {
    ms_delay(500);
    LEDPORT->ODR ^= (1 << LED1); // toggle diodes
  }

  return 0;
}