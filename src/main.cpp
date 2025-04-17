#include "stm32f030x6.h"

#include "usart.hpp"
#include "rcc.hpp"
#include "SignalBuffer.hpp"
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
  rcc::RCC_init();
  rcc::SYSTICK_init();

  USART_Init(9600);

  send("hello world!\n");

  const uint16_t sync[] = {360, 11160, 0, 0, 0, 0, 0, 0};         // sync
  const uint16_t zero_bit[] = {360, 1080, 360, 1080, 0, 0, 0, 0}; // 0
  const uint16_t one_bit[] = {360, 1080, 1080, 360, 0, 0, 0, 0};  // 1

  auto test = ic::PeriodPattern<8>(sync);

  test.match_window(sync);

  ic::init_ic();

    while (1)
  {
    toggle_onboard();

    ms_delay(1000);

    send(rcc::getSystick());
    send('\n');
  }

  return 0;
}