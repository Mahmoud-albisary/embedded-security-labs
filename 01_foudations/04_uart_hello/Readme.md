# UART Hello

In this subproject, the goal is to send a text message from the TM4C123
LaunchPad using UART0.

The linker script, `startup.c`, and `Makefile` are the same style as in the
previous projects. The project name is:

```make
PROJECT = uart_hello
```

The important changes are in `main.c`.

## What This Project Does

The board uses UART0 to transmit this message:

```c
const char *message = "Hello, UART!\r\n";
```

UART0 is connected to GPIO Port A:

- PA0 is UART0 RX.
- PA1 is UART0 TX.

This program configures PA0 and PA1 for their UART alternate function, sets the
UART baud rate, enables UART0, and then sends the message one character at a
time.

## Register Definitions

The project uses direct memory-mapped register access:

```c
#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define SYSCTL_RCGCUART_R   (*((volatile uint32_t *)0x400FE618))
#define GPIO_PORTA_DEN_R    (*((volatile uint32_t *)0x4000451C))
#define GPIO_PORTA_PCTL_R   (*((volatile uint32_t *)0x4000452C))
#define GPIO_PORTA_AFSEL_R  (*((volatile uint32_t *)0x40004420))
#define GPIO_PORTA_AMSEL_R  (*((volatile uint32_t *)0x40004528))
```

These names represent hardware registers used to configure GPIO Port A:

- `SYSCTL_RCGCGPIO_R` enables the GPIO peripheral clock.
- `SYSCTL_RCGCUART_R` enables the UART peripheral clock.
- `GPIO_PORTA_DEN_R` enables digital I/O on Port A pins.
- `GPIO_PORTA_PCTL_R` selects the peripheral function for Port A pins.
- `GPIO_PORTA_AFSEL_R` enables alternate functions on Port A pins.
- `GPIO_PORTA_AMSEL_R` disables or enables analog mode on Port A pins.

The UART0 registers are also defined directly:

```c
#define UART0_CTL_R         (*((volatile uint32_t *)0x4000C030))
#define UART0_IBRD_R        (*((volatile uint32_t *)0x4000C024))
#define UART0_FBRD_R        (*((volatile uint32_t *)0x4000C028))
#define UART0_LCRH_R        (*((volatile uint32_t *)0x4000C02C))
#define UART0_CC_R          (*((volatile uint32_t *)0x4000CFC8))
#define UART0_DR_R          (*((volatile uint32_t *)0x4000C000))
#define UART0_FR_R          (*((volatile uint32_t *)0x4000C018))
```

These registers control UART0:

- `UART0_CTL_R` enables or disables UART0, transmit, and receive.
- `UART0_IBRD_R` stores the integer part of the baud rate divisor.
- `UART0_FBRD_R` stores the fractional part of the baud rate divisor.
- `UART0_LCRH_R` configures the UART frame format.
- `UART0_CC_R` selects the UART clock source.
- `UART0_DR_R` is the data register used to send or receive bytes.
- `UART0_FR_R` is the flag register used to check UART status.

## Clock Setup

First, the firmware enables the GPIO Port A clock:

```c
#define GPIO_PORTA_CLOCK_EN (1U << 0)

SYSCTL_RCGCGPIO_R |= GPIO_PORTA_CLOCK_EN;
```

Port A must be enabled because UART0 uses PA0 and PA1.

Then the firmware enables the UART0 clock:

```c
SYSCTL_RCGCUART_R |= (1U << 0);
```

After enabling the clocks, the code reads the GPIO clock register once:

```c
volatile uint32_t dummy = SYSCTL_RCGCGPIO_R;
(void)dummy;
```

This gives the peripheral clock a short time to become active before the code
starts configuring GPIO registers.

## GPIO Setup for UART0

PA0 and PA1 are enabled as digital pins:

```c
GPIO_PORTA_DEN_R |= (1U << 0) | (1U << 1);
```

The code selects the UART function for PA0 and PA1 using `GPIO_PORTA_PCTL_R`:

```c
GPIO_PORTA_PCTL_R |= (1U << 0) | (1U << 4);
```

Each pin in `GPIO_PORTA_PCTL_R` uses four bits. For UART0:

- PA0 needs function value `1`.
- PA1 needs function value `1`.

That is why bit 0 and bit 4 are set.

The alternate function is then enabled for both pins:

```c
GPIO_PORTA_AFSEL_R |= (1U << 0) | (1U << 1);
```

Finally, analog mode is disabled on PA0 and PA1:

```c
GPIO_PORTA_AMSEL_R &= ~((1U << 0) | (1U << 1));
```

UART is a digital peripheral, so the pins should not be in analog mode.

## Baud Rate Setup

The UART baud rate is configured with two registers:

```c
UART0_IBRD_R = 8;
UART0_FBRD_R = 44;
```

`UART0_IBRD_R` stores the integer divisor and `UART0_FBRD_R` stores the
fractional divisor.

These values are used for 115200 baud when the UART clock is 16 MHz.

## UART Frame Format

The line control register configures the UART data format:

```c
UART0_LCRH_R = (1U << 5) | (1U << 6);
```

Bits 5 and 6 set the word length to 8 bits. The code does not enable parity, so
the frame format is:

- 8 data bits
- no parity
- 1 stop bit

This is commonly written as `8N1`.

## UART Clock Source

The clock source is selected with:

```c
UART0_CC_R = 0;
```

This tells UART0 to use the system clock.

## Enabling UART0

After the GPIO pins and UART settings are configured, the UART is enabled:

```c
UART0_CTL_R |= (1U << 0) | (1U << 8) | (1U << 9);
```

These bits enable:

- bit 0: UART0 itself
- bit 8: transmit
- bit 9: receive

Even though this program only sends data, receive is also enabled because PA0 is
configured as UART0 RX.

## Sending the Message

The message is stored as a string:

```c
const char *message = "Hello, UART!\r\n";
```

The `\r\n` at the end sends carriage return and newline. This usually makes the
terminal move to the beginning of the next line after printing the message.

The code sends one character at a time:

```c
while (*message) {
    while (UART0_FR_R & (1U << 5)) {
        // Wait until the transmit FIFO is not full
    }
    UART0_DR_R = *message++;
}
```

`*message` reads the current character. The loop continues until it reaches the
null terminator at the end of the string.

Before writing each character, the code checks bit 5 in `UART0_FR_R`.

Bit 5 is the TXFF flag:

- `1` means the transmit FIFO is full.
- `0` means there is space to write another byte.

So this loop waits while the transmit FIFO is full:

```c
while (UART0_FR_R & (1U << 5)) {
}
```

When there is space, the current character is written to the UART data register:

```c
UART0_DR_R = *message++;
```

The `++` moves the pointer to the next character after the current character is
sent.

## Program End

After all characters are written, `main` returns:

```c
return 0;
```

On a microcontroller project, programs usually run forever. This example only
sends one message, so after the string is transmitted there is no more
application work to do.
