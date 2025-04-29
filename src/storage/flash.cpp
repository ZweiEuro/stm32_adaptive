#include "storage/flash.hpp"
#include "stm32f030x6.h"

#include "sys/printf_getchar.hpp"
#include "rcc.hpp"
#include "util.hpp"
namespace flash
{

    // Raw flash operations

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

        FLASH_WaitForLastOperation();

        if ((FLASH->CR & FLASH_CR_LOCK) != 0)
        {
            FLASH->KEYR = FLASH_KEY1;
            FLASH->KEYR = FLASH_KEY2;
        }

        if (READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0)
        {
            printf("[ERR] flash unlock error\n");
        }
    }

    void flash_lock()
    {

        SET_BIT(FLASH->CR, FLASH_CR_LOCK);
    }

    void erase_page(const uint8_t *page_start_addr)
    {
        FLASH_WaitForLastOperation();

        // PER = Page Erase Mode
        SET_BIT(FLASH->CR, FLASH_CR_PER);

        // Address to clear
        FLASH->AR = (uint32_t)page_start_addr;

        SET_BIT(FLASH->CR, FLASH_CR_STRT);

        if (FLASH_WaitForLastOperation() != 0)
        {
            printf("[ERR] could not erase flash? %X\n", FLASH->SR);
        }
        CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
    }

    void write_to_flash(const uint8_t *destination, const uint8_t *src, const uint32_t num_bytes)
    {
        FLASH_WaitForLastOperation();

        SET_BIT(FLASH->CR, FLASH_CR_PG);

        for (int half_word_index = 0; half_word_index < num_bytes / 2; half_word_index++)
        {
            ((__IO uint16_t *)destination)[half_word_index] = ((uint16_t *)src)[half_word_index];

            FLASH_WaitForLastOperation();
        }

        CLEAR_BIT(FLASH->CR, FLASH_CR_PG);
    }

    // interface from outside

    PeriodPattern *getPattern(int n)
    {

        if (__SEC_SIGNAL_PATTERNS_DATA_START[0] != 0xAB)
        {
            printf("[Err] no periods to get\n");
            return nullptr;
        }

        if (__SEC_SIGNAL_PATTERNS_DATA_START[1] <= n || n < 0)
        {
            printf("[Err] pattern index %d does not exist\n", n);
            return nullptr;
        }

        auto pattern = &((PeriodPattern *)(__SEC_SIGNAL_PATTERNS_DATA_START + 2))[n];

        if (pattern->_padding != 0)
        {
            printf("[Err] pattern at %p has no 0 padding?", pattern);
            return nullptr;
        }
        return pattern;
    }

    void savePatterns(PeriodPattern *p, int n)
    {
        flash_unlock();

        erase_page(__SEC_SIGNAL_PATTERNS_DATA_START);

        // write n and magic byte
        const uint8_t mbyte_and_number[] = {0xAB, n};
        write_to_flash(__SEC_SIGNAL_PATTERNS_DATA_START, mbyte_and_number, 2); // write the two bytes

        // write the actual data
        // offset by the 2 byte before
        write_to_flash((__SEC_SIGNAL_PATTERNS_DATA_START + 2), (uint8_t *)p, n * sizeof(PeriodPattern));

        flash_lock();
    }
}