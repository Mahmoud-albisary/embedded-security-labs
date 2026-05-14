# UART Echo

Here we are exploring how to get data as a text from pc through UART and transmit it back.

This project runs on the Tiva C LaunchPad and uses UART0 to communicate with a computer over the board's USB debug connection. The firmware receives characters from the serial monitor, stores them in a small buffer, and sends the collected message back when SW1 is pressed.

## What this project demonstrates

- Configuring UART0 directly through memory-mapped registers.
- Using PA0 as `U0RX` and PA1 as `U0TX`.
- Receiving text from a computer through a serial monitor.
- Storing incoming characters in firmware.
- Triggering the echo response with the LaunchPad SW1 button.

## Project layout

```text
01_uart_echo/
├── Makefile
├── Readme.md
├── linker/
│   └── tm4c123gh6pm.ld
└── src/
    ├── main.c
    └── startup.c
```

## Main firmware flow

The important logic lives in `src/main.c`.

First, the firmware enables the clocks for GPIO Port A, GPIO Port F, and UART0:

- Port A is used for UART0 pins.
- Port F is used for the SW1 button.
- UART0 is used for serial communication with the computer.

Then it waits until the peripherals are ready before touching their registers. This matters because the clock enable register does not make the peripheral usable instantly.

After that, the firmware configures:

- PA0 as UART receive.
- PA1 as UART transmit.
- PF4 as SW1 input with an internal pull-up resistor.
- UART0 as `115200 8N1`.

Once initialized, it sends:

```text
UART echo ready
```

That message confirms that UART transmit is working.

## Runtime behavior

The main loop does two things:

1. Checks whether a UART character is available.
2. Checks whether SW1 was pressed.

UART receive is non-blocking in the main loop. Instead of waiting forever for a byte, the code first checks:

```c
uart_char_available()
```

If a character is available, it reads it and stores it in `buffer`.

When SW1 is pressed, the firmware sends the buffered message back:

```text
ECHO: <length> <message>
```

For example, if the computer sends:

```text
hello
```

and then SW1 is pressed, the board sends back:

```text
ECHO: 5 hello
```

## Serial monitor settings

Use these settings:

```text
Baud rate: 115200
Data bits: 8
Parity: none
Stop bits: 1
Flow control: none
```

On macOS, the serial device usually appears under:

```sh
ls /dev/cu.*
```

Then connect with:

```sh
screen /dev/cu.usbmodemXXXX 115200
```

Replace `/dev/cu.usbmodemXXXX` with the actual device name.

## Supporting files

The remaining files follow the same structure as the earlier foundation projects.

`src/startup.c` defines the minimal vector table and reset handler. It sets the initial stack pointer, jumps into `main`, and stays in an infinite loop if `main` ever returns.

`linker/tm4c123gh6pm.ld` describes the TM4C123 memory map, placing code in flash and data/BSS in SRAM.

`Makefile` builds the firmware with `arm-none-eabi-gcc`, generates the ELF and binary files, prints the firmware size, and provides a `clean` target.

## Current limitation

The buffer currently has a fixed size and does not protect against receiving too many characters before SW1 is pressed. That is intentional for now, but the next improvement should be to add bounds checking before writing into `buffer`.
