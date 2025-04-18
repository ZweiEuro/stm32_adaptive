#include "stm32f030x6.h"

#include "usart.hpp"
#include "rcc.hpp"
#include "SignalBuffer.hpp"
#include "input_capture.hpp"

#include "util.hpp"

namespace global
{
  int n_patterns = 0;
  ic::PeriodPattern **period_patterns = nullptr;
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

void add_pattern(ic::PeriodPattern *pattern)
{

  if (global::period_patterns == nullptr)
  {
    global::period_patterns = (ic::PeriodPattern **)calloc(sizeof(ic::PeriodPattern *), global::n_patterns);
  }
  static int index = 0;

  if (index >= global::n_patterns)
  {
    send("[ERR] patterns full");
  }

  global::period_patterns[index] = pattern;
  index++;
}

void init_patterns()
{

  global::n_patterns = 3;

  const uint16_t sync[] = {360, 11160, 0, 0, 0, 0, 0, 0};         // sync
  const uint16_t zero_bit[] = {360, 1080, 360, 1080, 0, 0, 0, 0}; // 0
  const uint16_t one_bit[] = {360, 1080, 1080, 360, 0, 0, 0, 0};  // 1

  add_pattern(new ic::PeriodPattern(sync, 0.3));
  add_pattern(new ic::PeriodPattern(zero_bit, 0.3));
  add_pattern(new ic::PeriodPattern(one_bit, 0.3));
}

// Alternates blue and green LEDs quickly
int main(void)
{

  init_patterns();

  setup_onboard_led();

  rcc::SYSTICK_init();
  rcc::RCC_init();

  USART_Init(9600);

  ic::init_ic();

  send("hello world!\n");

  while (1)
  {

    ic::process_signals();
  }

  return 0;
}