#include <stdint.h>

#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_RCGCUART_R   (*((volatile uint32_t *)0x400FE618))
#define SYSCTL_PRGPIO_R     (*((volatile uint32_t *)0x400FEA08))
#define SYSCTL_PRUART_R     (*((volatile uint32_t *)0x400FEA18))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))
#define GPIO_PORTF_DIR_R    (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DEN_R    (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_PUR_R    (*((volatile uint32_t *)0x40025510))
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
#define UART0_CLOCK_EN      (1U << 0)
#define SW1_BUTTON          (1U << 4)

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


void debounce_delay(void) {
    for (volatile uint32_t i = 0; i < 50000; i++) {
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

void uart_send_int(uint32_t num) {
    char buffer[11];
    int len = 0;
    if (num == 0) {
        uart_send_char('0');
        return;
    }
    while (num > 0) {
        buffer[len++] = '0' + (num % 10);
        num /= 10;
    }
    for (int i = len - 1; i >= 0; i--) {
        uart_send_char(buffer[i]);
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
    
    GPIO_PORTF_DIR_R &= ~SW1_BUTTON;
    GPIO_PORTF_DEN_R |= SW1_BUTTON;
    GPIO_PORTF_PUR_R |= SW1_BUTTON; // Enable pull-up resistor

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

    uart_send_string("UART echo ready\r\n");
    // uart_send_string("RX: ");
    char buffer[100];
    uint32_t index = 0;
    int sw1_pressed = 0;
    int prev_sw1_state = 0; // Not pressed
    while (1) {
        if (uart_char_available()) {
            char c = uart_receive_char();
            if (c != '\r' && c != '\n') {
                buffer[index] = c;
                index++;
            }
        }

        sw1_pressed = !(GPIO_PORTF_DATA_R & SW1_BUTTON);

        if (sw1_pressed && !prev_sw1_state) {
            debounce_delay();
            if(!(GPIO_PORTF_DATA_R & SW1_BUTTON)) {
                if (index == 0) {
                    prev_sw1_state = sw1_pressed;
                    continue;
                }

                buffer[index] = '\0';

                uart_send_string("ECHO: ");
                uart_send_int(index);
                uart_send_string(" ");
                for (uint32_t i = 0; i < index; i++) {
                    uart_send_char(buffer[i]);
                }

                uart_send_char('\r');
                uart_send_char('\n');

                index = 0;
            }
        }
        prev_sw1_state = sw1_pressed;
    }
    return 0;
}
