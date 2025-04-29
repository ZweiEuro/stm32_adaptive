#include "flash.hpp"
#include "stm32f030x6.h"

#include "usart.hpp"
#include "sys/printf.hpp"
#include "rcc.hpp"
#include "util.hpp"
namespace flash
{

#ifdef __cplusplus
    extern "C"
    {
#endif
        // comes from the linker and cannot be mangled
        extern uint8_t __SEC_SIGNAL_PATTERNS_DATA_START[1024];

        extern uint8_t __SEC_SYSTEM_RUN_DATA_START[1024];

#ifdef __cplusplus
    }
#endif

    inline int FLASH_WaitForLastOperation(uint32_t timeout = 0)
    {
        uint32_t tickstart = rcc::getSystick();

        while (READ_BIT(FLASH->SR, FLASH_SR_BSY))
        {
            if (timeout != 0)
            {
                if ((timeout == 0U) || ((rcc::getSystick() - tickstart) > timeout))
                {
                    printf("[ERR] timeout BSY flash");
                    return -1;
                }
            }
        }

        /* Check FLASH End of Operation flag  */
        if (READ_BIT(FLASH->SR, FLASH_SR_EOP))
        {
            /* Clear FLASH End of Operation pending bit */
            // CLEARED by writing _ 1 _ !!
            SET_BIT(FLASH->SR, FLASH_SR_EOP);
        }

        if (READ_BIT(FLASH->SR, FLASH_SR_WRPERR) ||
            READ_BIT(FLASH->SR, FLASH_SR_PGERR))
        {
            /*Save the error code*/
            printf("[ERR] Error flash op!");
            return -2;
        }

        /* There is no error flag set */
        return 0;
    }

    void flash_unlock()
    {

        /* (1) Wait till no operation is on going */
        /* (2) Check that the Flash is unlocked */
        /* (3) Perform unlock sequence */
        FLASH_WaitForLastOperation(); /* (1) */

        if ((FLASH->CR & FLASH_CR_LOCK) != 0) /* (2) */
        {
            FLASH->KEYR = FLASH_KEY1; /* (3) */
            FLASH->KEYR = FLASH_KEY2;
        }

        if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0)
        {
            printf("[ERR] flash unlock error\n");
        }

        printf("unlocked flash\n");
    }

    void flash_lock()
    {

        SET_BIT(FLASH->CR, FLASH_CR_LOCK);

        printf("locked flash\n");
    }

    void erase_page()
    {

        // erase the flash

        /* (1) Set the PER bit in the FLASH_CR register to enable page erasing */
        /* (2) Program the FLASH_AR register to select a page to erase */
        /* (3) Set the STRT bit in the FLASH_CR register to start the erasing */
        /* (4) Wait until the BSY bit is reset in the FLASH_SR register */
        /* (5) Check the EOP flag in the FLASH_SR register */
        /* (6) Clear EOP flag by software by writing EOP at 1 */
        /* (7) Reset the PER Bit to disable the page erase */
        FLASH->CR |= FLASH_CR_PER;                              /* (1) */
        FLASH->AR = (uint32_t)__SEC_SIGNAL_PATTERNS_DATA_START; /* (2) */
        FLASH->CR |= FLASH_CR_STRT;                             /* (3) */

        FLASH_WaitForLastOperation(); /* (4) */

        CLEAR_BIT(FLASH->CR, FLASH_CR_STRT);

        if (FLASH_WaitForLastOperation() != 0) // 5, 6
        {
            printf("[ERR] could not erase flash? %X\n", FLASH->SR);
        }

        FLASH->CR &= ~FLASH_CR_PER; /* (7) */

        printf("flash erased\n");
    }

    inline void write_half_word(uint32_t offset_on_page, uint16_t value)
    {
        FLASH_WaitForLastOperation();
        FLASH->CR |= FLASH_CR_PG;

        if (__SEC_SIGNAL_PATTERNS_DATA_START[offset_on_page] != 0xFF)
        {
            printf("Cannot write to non erased section!");
            return;
        }

        *(__IO uint16_t *)(&__SEC_SIGNAL_PATTERNS_DATA_START[offset_on_page]) = (uint16_t)value;

        if (FLASH_WaitForLastOperation() != 0) // 3, 4, 5
        {

            sendln("[ERR] could not write to flash");
        }

        FLASH->CR &= ~FLASH_CR_PG;
    }

    void program_flash_start()
    {
        printf("start write\n");

        for (int i = 0; i < sizeof(__SEC_SIGNAL_PATTERNS_DATA_START); i++)
        {

            if (__SEC_SIGNAL_PATTERNS_DATA_START[i] != 0xFF)
            {
                continue;
            }
            write_half_word(i, i);
            return;
        }
    }

    void test()
    {

        flash_unlock();

        {
            // prinf_arrln("%ld", (uint8_t *)__SEC_CONFIG_DATA_START, 10);
            // erase_page();
        }

        {
            // prinf_arrln("%ld", (uint8_t *)__SEC_CONFIG_DATA_START, 10);
        }
        program_flash_start();

        {
            prinf_arrln("%ld", (uint8_t *)__SEC_SIGNAL_PATTERNS_DATA_START, 10);
        }
    }
}