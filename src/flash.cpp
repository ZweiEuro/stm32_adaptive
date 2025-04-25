#include "flash.hpp"
#include "stm32f030x6.h"

#include "usart.hpp"
#include "sys/printf.hpp"

namespace flash
{

#ifdef __cplusplus
    extern "C"
    {
#endif
        // comes from the linker and cannot be mangled
        extern uint8_t __SEC_CONFIG_DATA_START[1024];

#ifdef __cplusplus
    }
#endif

#define WAIT_FOR_FLASH_N_BSY                  \
    while (READ_BIT(FLASH->SR, FLASH_SR_BSY)) \
    {                                         \
        ;                                     \
    }

    bool flash_locked = true;

    void flash_unlock()
    {

        if (flash_locked == false)
        {
            printf("flash already unlocked\n");
            // already unlocked
            return;
        }

        // unlock the flash

        /* (1) Wait till no operation is on going */
        /* (2) Check that the Flash is unlocked */
        /* (3) Perform unlock sequence */
        WAIT_FOR_FLASH_N_BSY; /* (1) */

        if ((FLASH->CR & FLASH_CR_LOCK) != 0) /* (2) */
        {
            FLASH->KEYR = FLASH_KEY1; /* (3) */
            FLASH->KEYR = FLASH_KEY2;
        }

        if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0)
        {
            printf("[ERR] flash unlock error\n");
        }

        flash_locked = false;

        PRINT_REG(FLASH->SR);

        printf("unlocked flash\n");
    }

    void flash_lock()
    {
        if (flash_locked == true)
        {
            printf("flash already locked\n");
            // already locked
            return;
        }
        FLASH->CR |= FLASH_CR_LOCK;

        printf("locking flash\n");
    }

    void erase_page()
    {

        { // erase the flash

            /* (1) Set the PER bit in the FLASH_CR register to enable page erasing */
            /* (2) Program the FLASH_AR register to select a page to erase */
            /* (3) Set the STRT bit in the FLASH_CR register to start the erasing */
            /* (4) Wait until the BSY bit is reset in the FLASH_SR register */
            /* (5) Check the EOP flag in the FLASH_SR register */
            /* (6) Clear EOP flag by software by writing EOP at 1 */
            /* (7) Reset the PER Bit to disable the page erase */
            FLASH->CR |= FLASH_CR_PER;                     /* (1) */
            FLASH->AR = (uint32_t)__SEC_CONFIG_DATA_START; /* (2) */
            FLASH->CR |= FLASH_CR_STRT;                    /* (3) */

            WAIT_FOR_FLASH_N_BSY; /* (4) */

            CLEAR_BIT(FLASH->CR, FLASH_CR_STRT);

            if ((FLASH->SR & FLASH_SR_EOP) != 0) /* (5) */
            {
                FLASH->SR = FLASH_SR_EOP; /* (6)*/
            }
            else
            {
                /* Manage the error cases */
                printf("[ERR] could not erase flash? %X\n", FLASH->SR);
            }
            FLASH->CR &= ~FLASH_CR_PER; /* (7) */
        }

        printf("flash erased\n");
    }

    void program_flash_start()
    {

        /* (1) Set the PG bit in the FLASH_CR register to enable programming */
        /* (2) Perform the data write (half-word) at the desired address */
        /* (3) Wait until the BSY bit is reset in the FLASH_SR register */
        /* (4) Check the EOP flag in the FLASH_SR register */
        /* (5) clear it by software by writing it at 1 */
        /* (6) Reset the PG Bit to disable programming */
        FLASH->CR |= FLASH_CR_PG; /* (1) */

        PRINT_REG(FLASH->CR);
        PRINT_REG(FLASH->SR);

        *(__IO uint16_t *)(__SEC_CONFIG_DATA_START) = 0; /* (2) */
        WAIT_FOR_FLASH_N_BSY;                            /* (3) */

        if ((FLASH->SR & FLASH_SR_EOP) != 0) /* (4) */
        {
            FLASH->SR = FLASH_SR_EOP; /* (5) */
        }
        else
        {
            /* Manage the error cases */
            sendln("[ERR] could not write to flash");
        }
        FLASH->CR &= ~FLASH_CR_PG; /* (6) */
    }

    void test()
    {
        printf("flash test start\n");
        PRINT_REG(FLASH->CR);
        flash_unlock();

        {
            // prinf_arrln("%ld", (uint8_t *)__SEC_CONFIG_DATA_START, 10);
            // erase_page();
        }

        {
            prinf_arrln("%ld", (uint8_t *)__SEC_CONFIG_DATA_START, 10);
        }
        program_flash_start();

        {
            prinf_arrln("%ld", (uint8_t *)__SEC_CONFIG_DATA_START, 10);
        }

        printf("flash test end");
    }
}