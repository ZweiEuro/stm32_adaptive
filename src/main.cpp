#include "stm32f030x6.h"

#include "usart.hpp"
#include "rcc.hpp"
#include "SignalBuffer.hpp"
#include "input_capture.hpp"
#include "config.hpp"

#include "util.hpp"

namespace global
{
  uint8_t found_signals[100] = {255};
}

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

  memset(global::found_signals, 255, sizeof(global::found_signals));

  static uint32_t found_index = 0;

  while (1)
  {
    conf::handle_usart();

    auto found = ic::process_signals();

    if (found != -1)
    {
      global::found_signals[found_index++] = found;
      toggle_onboard();
    }
  }

  return 0;
}