#include "flash.hpp"
#include "stm32f030x6.h"

#include "usart.hpp"

namespace flash
{

#ifdef __cplusplus
    extern "C"
    {
#endif
        // comes from the linker and cannot be mangled
        extern int __SEC_CONFIG_DATA_START;

#ifdef __cplusplus
    }
#endif

    uint8_t *SEC_CONFIG_DATA_START = static_cast<uint8_t *>((void *)&__SEC_CONFIG_DATA_START);

    const auto max_flash_size = 0x4000UL;
    const auto page_size = 0x08007FFF - 0x08007C00;

    void erase_page()
    {

        { // unlock the flash

            /* (1) Wait till no operation is on going */
            /* (2) Check that the Flash is unlocked */
            /* (3) Perform unlock sequence */
            while ((FLASH->SR & FLASH_SR_BSY) != 0) /* (1) */
            {
                ;
                /* For robust implementation, add here time-out management */
            }
            if ((FLASH->CR & FLASH_CR_LOCK) != 0) /* (2) */
            {
                FLASH->KEYR = FLASH_KEY1; /* (3) */
                FLASH->KEYR = FLASH_KEY2;
            }
        }

        { // erase the flash

            /* (1) Set the PER bit in the FLASH_CR register to enable page erasing */
            /* (2) Program the FLASH_AR register to select a page to erase */
            /* (3) Set the STRT bit in the FLASH_CR register to start the erasing */
            /* (4) Wait until the BSY bit is reset in the FLASH_SR register */
            /* (5) Check the EOP flag in the FLASH_SR register */
            /* (6) Clear EOP flag by software by writing EOP at 1 */
            /* (7) Reset the PER Bit to disable the page erase */
            FLASH->CR |= FLASH_CR_PER;                   /* (1) */
            FLASH->AR = (uint32_t)SEC_CONFIG_DATA_START; /* (2) */
            FLASH->CR |= FLASH_CR_STRT;                  /* (3) */
            while ((FLASH->SR & FLASH_SR_BSY) != 0)      /* (4) */
            {

                /* For robust implementation, add here time-out management */
                ;
            }
            if ((FLASH->SR & FLASH_SR_EOP) != 0) /* (5) */
            {
                FLASH->SR = FLASH_SR_EOP; /* (6)*/
            }
            else
            {
                /* Manage the error cases */
                sendln("[ERR] could not erase flash?");
                send(FLASH->SR);
                sendln();
            }
            FLASH->CR &= ~FLASH_CR_PER; /* (7) */
        }

        sendln("flash erased");
    }

    void test()
    {
        sendln("flash test");
        send((uint32_t)SEC_CONFIG_DATA_START);
        sendln();

        {
            send_array((uint8_t *)SEC_CONFIG_DATA_START, 10);
        }

        erase_page();

        {
            send_array((uint8_t *)SEC_CONFIG_DATA_START, 10);
        }
    }
}