#include "stm32f030x6.h"

#include "rcc.hpp"
#include "SignalBuffer.hpp"
#include "input_capture.hpp"
#include "interface.hpp"
#include "sender.hpp"

#include "sys/printf_getchar.hpp"
#include "util.hpp"
#include "storage/flash.hpp"
#include "sys/usart.hpp"

#include "interface.hpp"

#include "main.hpp"

#ifdef FEATURE_WS2815_STRIP_ENABLED
#include "features/ws28xx.hpp"
#endif

// Alternates blue and green LEDs quickly
int main(void)
{

  rcc::SYSTICK_init();
  rcc::RCC_init();

  usart::init(9600);

  printf("Hello world!\n");

  sender::setup();

  ic::init_ic();

  while (1)
  {

    interface::handle_usart();

    auto found = ic::process_signals();

    if (found != -1)
    {
      interface::found_indices.push(found);
    }
  }

  return 0;
}