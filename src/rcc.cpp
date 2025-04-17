#include "rcc.hpp"
#include "usart.hpp"

namespace rcc
{

    // RCC initialization
    void RCC_init(void)
    {
        /* Enable Prefetch Buffer */
        FLASH->ACR |= FLASH_ACR_PRFTBE;
        /* Flash 1 wait state */
        FLASH->ACR |= FLASH_ACR_LATENCY;
#ifdef INTERNAL_OSCILLATOR
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
#else
        /* Enable HSE */
        RCC->CR |= RCC_CR_HSEON;
        /* Wait for HSE to be ready */
        while (!(RCC->CR & RCC_CR_HSERDY))
            ;
        /* Peripheral Clock divisors */
        RCC->CFGR2 |= RCC_CFGR2_PREDIV_DIV1;
        /* PLLCLK */
        RCC->CFGR &= ~(RCC_CFGR_PLLSRC);
        RCC->CFGR &= RCC_CFGR_PLLMUL;
        RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLMUL6);
#endif

#if 0 // enable PLL

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
#else
        /* Disable PLL */
        RCC->CR &= ~RCC_CR_PLLON;

        /* Select PLL as system Clock */
        RCC->CFGR &= ~RCC_CFGR_SW;
        RCC->CFGR |= RCC_CFGR_SW_HSE;
        /* Wait for HSE to become system core clock */
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE)
            ;
#endif
    }

    void SYSTICK_init(void)
    {
        return;
        SysTick->VAL = 0;

        SysTick->CTRL |= 1 << SysTick_CTRL_CLKSOURCE_Pos; // select core
        SysTick->LOAD = (F_CPU / 1000) - 1;

        NVIC_EnableIRQ(SysTick_IRQn);

        // enable the sys tick
        SysTick->CTRL |= (1 << SysTick_CTRL_ENABLE_Pos);

        // enable sys tick interrupt
        SysTick->CTRL |= (1 << SysTick_CTRL_TICKINT_Pos);
    }

    uint32_t systick_ms = 0;

    uint32_t getSystick()
    {

        return systick_ms;
    }

#ifdef __cplusplus
    extern "C"
    {
#endif
        void SysTick_IRQHandler(void)
        {
            systick_ms++;
        }

#ifdef __cplusplus
    }
#endif
}
