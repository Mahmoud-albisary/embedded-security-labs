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