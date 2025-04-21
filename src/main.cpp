#include "stm32f030x6.h"

#include "usart.hpp"
#include "rcc.hpp"
#include "SignalBuffer.hpp"
#include "input_capture.hpp"
#include "config.hpp"

#include "util.hpp"

void setup_onboard_led()
{
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;   // enable the clock to GPIO
  GPIOA->MODER |= GPIO_MODER_MODER4_0; // set pins to be general purpose output
}

void toggle_onboard()
{
  GPIOA->ODR ^= (1 << 4);
}

// Alternates blue and green LEDs quickly
int main(void)
{
  setup_onboard_led();

  rcc::SYSTICK_init();
  rcc::RCC_init();

  USART_Init(9600);

  ic::init_ic();

  while (1)
  {

    // ic::process_signals();

    conf::handle_usart();
  }

  return 0;
}