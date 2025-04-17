#include "stm32f030x6.h"

#include "usart.hpp"
#include "rcc.h"
#include "ringbuffer.hpp"
#include "input_capture.hpp"

void setup_onboard_led()
{
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;   // enable the clock to GPIO
  GPIOA->MODER |= GPIO_MODER_MODER4_0; // set pins to be general purpose output
}

void toggle_onboard()
{
  GPIOA->ODR ^= (1 << 4);
}

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
  setup_onboard_led();

  char buffer[64];
  RCC_Init();
  USART_Init(9600);

  send("hello world!\n");

  ic::init_ic();

  while (1)
  {
    toggle_onboard();

    ms_delay(1000);
  }

  return 0;
}