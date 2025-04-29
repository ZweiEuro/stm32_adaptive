#include "stm32f030x6.h"

#include "rcc.hpp"
#include "SignalBuffer.hpp"
#include "input_capture.hpp"
#include "interface.hpp"
#include "sender.hpp"

#include "sys/printf_getchar.hpp"
#include "util.hpp"
#include "flash.hpp"
#include "sys/usart.hpp"

namespace global
{
  uint8_t found_signals[100] = {255};
}

void setup_pinouts()
{
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // enable the clock to GPIO

  // PA4
  GPIOA->MODER |= GPIO_MODER_MODER4_0; // set pins to be general purpose output

  // PA0
  GPIOA->MODER |= GPIO_MODER_MODER0_0;
}

const auto PIN_PA4_Pos = (1 << 4);

void toggle_onboard()
{
  GPIOA->ODR ^= PIN_PA4_Pos;
}

extern uint8_t __SEC_CONFIG_DATA_START[1024];
// Alternates blue and green LEDs quickly
int main(void)
{
  setup_pinouts();

  rcc::SYSTICK_init();
  rcc::RCC_init();

  usart::init(9600);

  memset(global::found_signals, 255, sizeof(global::found_signals));

  static uint32_t found_index = 0;

  printf("Hello world!\n");

  sender::setup();

  TIM14->ARR = 256;
  TIM14->CR1 |= TIM_CR1_CEN;
  while (1)
  {
    ;
  }

  ic::init_ic();

  while (1)
  {

    interface::handle_usart();

    auto found = ic::process_signals();

    if (found != -1)
    {
      global::found_signals[found_index++] = found;
      toggle_onboard();
    }
  }

  return 0;
}