#include "rcc.hpp"

namespace rcc
{

#define core_mhz (F_CPU / 1000000)

    /**
     * Call to activate the internal HSI and set it as source
     */
    void use_internal_oscilator()
    {
        /* Enable HSI */
        RCC->CR |= RCC_CR_HSION;
        /* Wait for HSI to be ready */
        while (!(RCC->CR & RCC_CR_HSIRDY))
            ;
        /* Peripheral Clock divisors */
        RCC->CFGR2 = RCC_CFGR2_PREDIV_DIV1;
        /* PLLCLK */
        RCC->CFGR &= ~RCC_CFGR_PLLSRC;
        RCC->CFGR &= ~RCC_CFGR_PLLMUL;
        RCC->CFGR |= RCC_CFGR_PLLMUL12;
    }

    // RCC initialization
    void
    RCC_init(void)
    {
        /* Enable Prefetch Buffer */
        FLASH->ACR |= FLASH_ACR_PRFTBE;
        /* Flash 1 wait state */
        // we run under 8mhz, so latency is not needed!

        CLEAR_BIT(FLASH->ACR, 0b111 << FLASH_ACR_LATENCY_Pos); // clear latency
#if core_mhz > 24
        // IF         24 MHz < SYSCLK ≤ 48 MHz
        FLASH->ACR |= FLASH_ACR_LATENCY;
#endif

        /* Enable HSE */
        RCC->CR |= RCC_CR_HSEON;
        /* Wait for HSE to be ready */
        while (!(RCC->CR & RCC_CR_HSERDY))
            ;
        /* Peripheral Clock divisors */
        RCC->CFGR2 |= RCC_CFGR2_PREDIV_DIV1;

#if core_mhz == 8

        /* Disable PLL */
        RCC->CR &= ~RCC_CR_PLLON;

        /* Select HSE as system Clock */
        RCC->CFGR &= ~RCC_CFGR_SW;
        RCC->CFGR |= RCC_CFGR_SW_HSE;
        /* Wait for HSE to become system core clock */
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE)
            ;

#else

        // if we want 48 Mhz we can upscale the 8mhz HSE with the PLL

        /* PLLCLK */
        RCC->CFGR &= ~(0b11 << RCC_CFGR_PLLSRC_Pos); // clear the source selection for the PLL
        RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE_PREDIV);   // set as source, the HSE clock _before_ division (our division is 1 but anyways...)

        // clear multiplication
        RCC->CFGR &= ~(0b1111 << RCC_CFGR_PLLMUL_Pos); // clear (this means x2)

#if core_mhz == 16
        // we area already in this state but whatever
        RCC->CFGR |= (0b0000 << RCC_CFGR_PLLMUL_Pos);
#elif core_mhz == 32
        // 8 mhz extenrl * 4 = 32 mhz
        RCC->CFGR |= (0b00010 << RCC_CFGR_PLLMUL_Pos);
#elif core_mhz == 40
        // 8 mhz extenrl * 5 = 40 mhz
        RCC->CFGR |= (0b00011 << RCC_CFGR_PLLMUL_Pos);
#elif core_mhz == 48
        // 8 mhz extenrl * 6 = 48 mhz
        RCC->CFGR |= (0b00100 << RCC_CFGR_PLLMUL_Pos);
#else
#error "Invalid clock value for PLL"
#endif

        /* Enable PLL */
        RCC->CR |= RCC_CR_PLLON;
        /* Wait until the PLL is ready */
        while (!(RCC->CR & RCC_CR_PLLRDY))
            ;
        /* Select PLL as system Clock */
        RCC->CFGR &= ~RCC_CFGR_SW;
        RCC->CFGR |= RCC_CFGR_SW_PLL;
        /* Wait for PLL to become system core clock */
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
            ;

#endif
    }

    void SYSTICK_init(void)
    {

        SysTick->VAL = 1;

        SysTick->LOAD = (F_CPU / 1000) - 1;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk; // select core

        NVIC_EnableIRQ(SysTick_IRQn);

        // enable the sys tick
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

        // enable sys tick interrupt
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
    }

    uint64_t systick_ms = 0;

    // Sys time in ms
    uint64_t getSystick()
    {

        return systick_ms;
    }

#ifdef __cplusplus
    extern "C"
    {
#endif
        void SysTick_Handler(void)
        {
            systick_ms++;
        }

#ifdef __cplusplus
    }
#endif
}
