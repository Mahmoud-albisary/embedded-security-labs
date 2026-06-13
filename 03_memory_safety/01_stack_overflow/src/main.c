#include <stdint.h>

#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_PRGPIO_R     (*((volatile uint32_t *)0x400FEA08))
#define GPIO_PORTF_DIR_R    (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DEN_R    (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))

#define GPIO_PORTF_CLOCK_EN (1U << 5)
#define RED_LED             (1U << 1)

static void blink(void);
void foo() {
    volatile uint32_t a = 0xAAAAAAAA;
    volatile uint32_t buffer[10];

    // stack buffer overflow with silent data corruption
    buffer[10] = 293; // This will cause a stack overflow
    volatile uint32_t b = 0xBBBBBBBB;

    // stack buffer overflow with an invalid-address BusFault
    buffer[20] = 123; // This will cause another stack overflow

    while(1);
}

// Do not run this function, as it will cause a stack overflow due to infinite recursion. 
// Uncommenting this function and calling it will lead to a stack overflow, which can cause the program to crash or behave unpredictably.
// void bar() {
//     volatile uint32_t c = 0xCCCCCCCC;
//     bar(); // This will cause a stack overflow due to infinite recursion
// }

volatile uint32_t observation;

__attribute__((noinline))

static void helper(void)
{
    observation++;
}

__attribute__((noinline))

static void vulnerable(void)
{
    volatile uint32_t buffer[10];
    buffer[0] = 0;

    helper();  /* Forces vulnerable() to preserve its own return address. */
    buffer[11] = ((uint32_t)(uintptr_t)&blink) | 1U;  /* This will cause a stack overflow, overwriting the return address. */
    /*
     * Initially leave all out-of-bounds writes disabled.
     * First inspect the generated frame using GDB.
     */
    __asm volatile ("nop");
}

static void delay(void) {
    for (volatile uint32_t i = 0; i < 1000000; i++) {
    }
}

static void blink(void) {
    SYSCTL_RCGCGPIO_R |= GPIO_PORTF_CLOCK_EN;
        /* Wait until Port F is reported ready before accessing its registers. */
    while ((SYSCTL_PRGPIO_R & GPIO_PORTF_CLOCK_EN) == 0U) {
    }

    // Small delay after enabling peripheral clock
    volatile uint32_t dummy = SYSCTL_RCGCGPIO_R;
    (void)dummy;

    GPIO_PORTF_DIR_R |= RED_LED;
    GPIO_PORTF_DEN_R |= RED_LED;

    while (1) {
        GPIO_PORTF_DATA_R ^= RED_LED;
        delay();
    }
}

int main(void) {
    vulnerable();
    //bar();
    return 0;
}