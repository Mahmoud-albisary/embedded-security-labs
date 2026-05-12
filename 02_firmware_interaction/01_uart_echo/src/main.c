#include <stdint.h>

#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_RCGCUART_R   (*((volatile uint32_t *)0x400FE618))
#define SYSCTL_PRGPIO_R     (*((volatile uint32_t *)0x400FEA08))
#define SYSCTL_PRUART_R     (*((volatile uint32_t *)0x400FEA18))
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
#define UART0_CLOCK_EN      (1U << 0)

void uart_send_char(char c) {
    while (UART0_FR_R & (1U << 5)) {
        // Wait until the transmit FIFO is not full
    }
    UART0_DR_R = c;
}

char uart_receive_char() {
    while (UART0_FR_R & (1U << 4)) {
        // Wait until the receive FIFO is not empty
    }
    return UART0_DR_R & 0xFF; // Read the received character
}

int main(void) {
    SYSCTL_RCGCGPIO_R |= GPIO_PORTA_CLOCK_EN;
    SYSCTL_RCGCUART_R |= UART0_CLOCK_EN;

    while ((SYSCTL_PRGPIO_R & GPIO_PORTA_CLOCK_EN) == 0) {
    }
    while ((SYSCTL_PRUART_R & UART0_CLOCK_EN) == 0) {
    }

    UART0_CTL_R &= ~(1U << 0); // Disable UART0 before configuration

    GPIO_PORTA_DIR_R = (GPIO_PORTA_DIR_R & ~(1U << 0)) | (1U << 1);
    GPIO_PORTA_DEN_R |= (1U << 0) | (1U << 1);
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & ~0xFFU) | 0x11U;
    GPIO_PORTA_AFSEL_R |= (1U << 0) | (1U << 1);
    GPIO_PORTA_AMSEL_R &= ~((1U << 0) | (1U << 1)); // Disable analog on PA0 and PA1

    UART0_IBRD_R = 8; // Integer part of baud rate divisor for 115200 baud
    UART0_FBRD_R = 44;  // Fractional part of baud rate divisor for 115200 baud
    UART0_LCRH_R = (1U << 5) | (1U << 6); // 8-bit, no parity, one stop bit
    UART0_CC_R = 0; // Use system clock
    UART0_CTL_R |= (1U << 0) | (1U << 8) | (1U << 9); // Enable UART, TX, and RX

    while (1) {
        char c = uart_receive_char();
    }
    return 0;
}
