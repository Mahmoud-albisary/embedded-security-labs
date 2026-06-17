#include <stdint.h>
#include <stdbool.h>

#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_RCGCUART_R   (*((volatile uint32_t *)0x400FE618))
#define SYSCTL_PRGPIO_R     (*((volatile uint32_t *)0x400FEA08))
#define SYSCTL_PRUART_R     (*((volatile uint32_t *)0x400FEA18))
#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_PRGPIO_R     (*((volatile uint32_t *)0x400FEA08))
#define GPIO_PORTF_DIR_R    (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DEN_R    (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))
#define GPIO_PORTA_DIR_R    (*((volatile uint32_t *)0x40004400))
#define GPIO_PORTA_DEN_R    (*((volatile uint32_t *)0x4000451C))
#define GPIO_PORTA_PCTL_R   (*((volatile uint32_t *)0x4000452C))
#define GPIO_PORTA_AFSEL_R  (*((volatile uint32_t *)0x40004420))
#define GPIO_PORTA_CTL_R    (*((volatile uint32_t *)0x4000452C))
#define GPIO_PORTA_AMSEL_R  (*((volatile uint32_t *)0x40004528))
#define UART0_CTL_R         (*((volatile uint32_t *)0x4000C030))
#define UART0_IBRD_R        (*((volatile uint32_t *)0x4000C024))
#define UART0_FBRD_R        (*((volatile uint32_t *)0x4000C028))
#define UART0_LCRH_R        (*((volatile uint32_t *)0x4000C02C))
#define UART0_CC_R          (*((volatile uint32_t *)0x4000CFC8))
#define UART0_DR_R          (*((volatile uint32_t *)0x4000C000))
#define UART0_FR_R          (*((volatile uint32_t *)0x4000C018))

#define GPIO_PORTA_CLOCK_EN (1U << 0)
#define GPIO_PORTF_CLOCK_EN (1U << 5)
#define RED_LED             (1U << 1)
#define UART0_CLOCK_EN      (1U << 0)

static void blink(void);
static bool last_char_was_newline = false;

const uintptr_t __stack_chk_guard = 0xA5A5A5A5u;

__attribute__((noreturn, no_stack_protector))
void __stack_chk_fail(void)
{
    while (1)
    {
        /* Stack smashing detected. Break here in GDB. */
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
    buffer[11] = 0xA1;  // ((uint32_t)(uintptr_t)&blink) | 1U; /* This will cause a stack overflow, overwriting the return address. */
    /*
     * Initially leave all out-of-bounds writes disabled.
     * First inspect the generated frame using GDB.
     */
    __asm volatile ("nop");
}

void uart_send_char(char c) {
    while (UART0_FR_R & (1U << 5)) {
        // Wait until the transmit FIFO is not full
    }
    UART0_DR_R = c;
}

void uart_send_string(const char *message) {
    while (*message) {
        uart_send_char(*message++);
    }
}

char uart_receive_char() {
    while (UART0_FR_R & (1U << 4)) {
        // Wait until the receive FIFO is not empty
    }
    return UART0_DR_R & 0xFF; // Read the received character
}

int uart_char_available(void) {
    return (UART0_FR_R & (1U << 4)) == 0;
}

uint32_t uart_read_hex_word(void) {
    uint32_t value = 0;
    last_char_was_newline = false;
    while (1) {
        char c = uart_receive_char();
        if (c == '\r' || c == '\n') {
            last_char_was_newline = true;
            break;
        }
        if(c == ' ') {
            break;
        }
        value <<= 4;
        if (c >= '0' && c <= '9') {
            value |= (uint32_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            value |= (uint32_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            value |= (uint32_t)(c - 'A' + 10);
        }
    }
    return value;
}

bool uart_last_char_was_newline(void) {
    return last_char_was_newline;
}

void vulnerable2(void) {
    volatile uint32_t buffer[8];
    uint32_t i = 0;

    GPIO_PORTA_DIR_R = (GPIO_PORTA_DIR_R & ~(1U << 0)) | (1U << 1);
    GPIO_PORTA_DEN_R |= (1U << 0) | (1U << 1);
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & ~0xFFU) | 0x11U;
    GPIO_PORTA_AFSEL_R |= (1U << 0) | (1U << 1);
    GPIO_PORTA_AMSEL_R &= ~((1U << 0) | (1U << 1));

    UART0_CTL_R &= ~(1U << 0); // Disable UART0 before configuration

    UART0_IBRD_R = 8; // Integer part of baud rate divisor for 115200 baud
    UART0_FBRD_R = 44;  // Fractional part of baud rate divisor for 115200 baud
    UART0_LCRH_R = (1U << 5) | (1U << 6); // 8-bit, no parity, one stop bit
    UART0_CC_R = 0; // Use system clock
    UART0_CTL_R |= (1U << 0) | (1U << 8) | (1U << 9); // Enable UART, TX, and RX
    uart_send_string("Enter 32-bit words in hex, end with newline:\r\n");

    while (1) {
        uint32_t word = uart_read_hex_word(); //payload input: "11111111 22222222 33333333 44444444 55555555 66666666 77777777 88888888 88888888 00000009 20007ff0 000002e9\r"
        //Note this payload may change every build due to ASLR, so adjust the last two words accordingly based on the observed stack layout in GDB.
        buffer[i] = word;   // normal-looking indexed input
        i++;
        if (uart_last_char_was_newline()) {
            uart_send_string("NEWLINE DETECTED\r\n");
            break;
        }
    }
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
    SYSCTL_RCGCGPIO_R |= GPIO_PORTA_CLOCK_EN | GPIO_PORTF_CLOCK_EN;
    SYSCTL_RCGCUART_R |= UART0_CLOCK_EN;

    while ((SYSCTL_PRGPIO_R & (GPIO_PORTA_CLOCK_EN | GPIO_PORTF_CLOCK_EN)) !=
           (GPIO_PORTA_CLOCK_EN | GPIO_PORTF_CLOCK_EN)) {
    }
    while ((SYSCTL_PRUART_R & UART0_CLOCK_EN) == 0) {
    }
    vulnerable2();
    //bar();
    return 0;
}