/**
 * @file    FaultHandler.c
 * @author  Ferenc Nemeth
 * @date    12 Aug 2018
 * @brief   This is the fault handler, where the program gets, if there is a problem.
 *          In case of a Hard fault, the following actions are going to be executed:
 *          - Prints out the stack frame registers (R0-R3, R12, LR, PC, PSR).
 *          - Prints out the fault status registers (HFSR, CFSR, MMFAR, BFAR, AFSR).
 *          - Prints out the EXC_RETURN register.
 *          - After these, the software analyzes the value of the registers
 *            and prints out a report regarding the fault.
 *          - Stop the execution with a breakpoint.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 */

#include "sys/FaultHandler.hpp"
#include "sys/printf_getchar.hpp"

/* Local function. */

#ifdef __cplusplus
extern "C"
{
#endif
    inline void ReportHardFault(uint32_t *hard_fault_values, uint32_t exc);

    /**
     * @brief   The program jumps here in case of a Hard fault.
     *          Checks which stack (MSP or PSP) was used.
     *          Extracts the stack frame location (and EXC_return),
     *          then pass them to ReportHardFault().
     * @param   void
     * @return  void
     */
    void HardFault_Handler(void)
    {

#if false
        printf("\n!!!Hard Fault detected!!!\n");

        while (true)
            ;

#else
        auto control_register = __get_CONTROL();

        uint32_t *stack_frame = nullptr;

        if (control_register & (1 << CONTROL_SPSEL_Pos))
        {
            stack_frame = (uint32_t *)__get_PSP();
        }
        else
        {
            stack_frame = (uint32_t *)__get_MSP();
        }

        ReportHardFault(stack_frame, control_register);

#endif
    }

    /**
     * @brief   Prints the registers and gives detailed information about the error(s).
     * @param   *stack_frame: Stack frame registers (R0-R3, R12, LR, LC, PSR).
     * @param   exc: EXC_RETURN register.
     * @return  void
     */
    inline void ReportHardFault(uint32_t *stack_frame, uint32_t exc)
    {
        uint32_t r0 = stack_frame[0];
        uint32_t r1 = stack_frame[1];
        uint32_t r2 = stack_frame[2];
        uint32_t r3 = stack_frame[3];
        uint32_t r12 = stack_frame[4];
        uint32_t lr = stack_frame[5];
        uint32_t pc = stack_frame[6];
        uint32_t psr = stack_frame[7];

        printf("\n!!!Hard Fault detected!!!\n");

        printf("\nStack frame:\n");
        printf("R0 :        0x%08lX\n", r0);
        printf("R1 :        0x%08lX\n", r1);
        printf("R2 :        0x%08lX\n", r2);
        printf("R3 :        0x%08lX\n", r3);
        printf("R12:        0x%08lX\n", r12);
        printf("LR :        0x%08lX\n", lr);
        printf("PC :        0x%08lX\n", pc);
        printf("PSR:        0x%08lX\n", psr);

        printf("\n Flash:\n");
        PRINT_REG(FLASH->ACR);
        PRINT_REG(FLASH->SR);
        PRINT_REG(FLASH->CR);
        PRINT_REG(FLASH->AR);
        PRINT_REG(FLASH->OBR);
        PRINT_REG(FLASH->WRPR);

        printf("\n RCC: \n");
        PRINT_REG(RCC->CR);

        __asm volatile("BKPT #0");

        while (1)
            ;
    }
#ifdef __cplusplus
}
#endif