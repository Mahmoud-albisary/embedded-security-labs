#include <stdint.h>

extern int main(void);

extern uint32_t _estack;

void Reset_Handler(void);
void Default_Handler(void);
void HardFault_Handler(void);

__attribute__((section(".isr_vector")))
uint32_t vector_table[] = {
    (uint32_t)&_estack,             /* Initial stack pointer */
    (uint32_t)Reset_Handler,        /* Reset */
    (uint32_t)Default_Handler,      /* NMI */
    (uint32_t)HardFault_Handler,    /* HardFault */
};

void Reset_Handler(void) {
    main();

    while (1) {
    }
}

void Default_Handler(void) {
    while (1) {
    }
}

volatile uint32_t hardfault_stack_pointer;
volatile uint32_t hardfault_r0;
volatile uint32_t hardfault_r1;
volatile uint32_t hardfault_r2;
volatile uint32_t hardfault_r3;
volatile uint32_t hardfault_r12;
volatile uint32_t hardfault_lr;
volatile uint32_t hardfault_pc;
volatile uint32_t hardfault_xpsr;

__attribute__((noreturn, no_stack_protector))
void hardfault_c(uint32_t *stack_frame)
{
    hardfault_stack_pointer = (uint32_t)stack_frame;
    hardfault_r0 = stack_frame[0];
    hardfault_r1 = stack_frame[1];
    hardfault_r2 = stack_frame[2];
    hardfault_r3 = stack_frame[3];
    hardfault_r12 = stack_frame[4];
    hardfault_lr = stack_frame[5];
    hardfault_pc = stack_frame[6];
    hardfault_xpsr = stack_frame[7];

    while (1)
    {
        /* HardFault captured. Inspect hardfault_pc and hardfault_lr in GDB. */
    }
}

__attribute__((naked, no_stack_protector))
void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4        \n"
        "ite eq            \n"
        "mrseq r0, msp     \n"
        "mrsne r0, psp     \n"
        "b hardfault_c     \n"
    );
}