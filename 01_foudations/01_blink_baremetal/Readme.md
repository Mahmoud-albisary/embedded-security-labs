# Blink Bare-Metal Project

This project is a minimal bare-metal firmware for the TM4C123/Tiva C
microcontroller. It enables GPIO Port F and toggles the red LED in an infinite
loop without using an operating system, vendor HAL, or C runtime startup code.

The goal is to show the smallest useful embedded flow:

1. The reset vector starts the firmware.
2. The startup code calls `main()`.
3. `main()` configures a hardware peripheral by writing directly to memory
   mapped registers.
4. The Makefile builds the firmware into an ELF file and a raw binary image.

## Files

```text
01_blink_baremetal/
├── Makefile
├── linker/
│   └── tm4c123gh6pm.ld
└── src/
    ├── main.c
    └── startup.c
```

## Makefile

The Makefile defines how the project is compiled and converted into firmware
outputs.

```make
PROJECT = blink_baremetal
```

This sets the output name. The build creates:

- `blink_baremetal.elf`: the linked executable with symbols and debug info.
- `blink_baremetal.bin`: the raw binary image that can be flashed.
- `blink_baremetal.map`: a linker map showing where code and data were placed.

```make
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
```

These are ARM embedded toolchain programs:

- `arm-none-eabi-gcc` compiles and links the firmware.
- `arm-none-eabi-objcopy` extracts a raw binary from the ELF file.
- `arm-none-eabi-size` prints the firmware size.

```make
CPU = cortex-m4
CFLAGS = -mcpu=$(CPU) -mthumb -Wall -Wextra -O0 -g -ffreestanding -nostdlib
```

Important compiler flags:

- `-mcpu=cortex-m4`: generate code for the ARM Cortex-M4 CPU.
- `-mthumb`: use the Thumb instruction set used by Cortex-M devices.
- `-Wall -Wextra`: enable useful compiler warnings.
- `-O0`: disable optimization, which makes early debugging easier.
- `-g`: include debug symbols in the ELF file.
- `-ffreestanding`: tell the compiler this is not a hosted desktop program.
- `-nostdlib`: do not link the normal C standard library startup code.

```make
LDFLAGS = -T linker/tm4c123gh6pm.ld -nostdlib -Wl,-Map=$(PROJECT).map
```

The linker flags use the custom linker script and generate the map file.
Because this is bare-metal firmware, the linker script decides where flash,
SRAM, the vector table, code, data, and stack are located.

```make
SRC = src/startup.c src/main.c
```

Both source files are compiled and linked together. `startup.c` provides the
reset entry point, and `main.c` contains the application logic.

```make
all: $(PROJECT).elf $(PROJECT).bin size
```

The default build target creates the ELF, converts it to a binary, and prints
the final size.

```make
clean:
	rm -f *.elf *.bin *.map
```

The `clean` target removes generated build outputs.

## startup.c

`startup.c` is the first C code involved in booting the firmware. On a
Cortex-M microcontroller, the CPU expects a vector table at the beginning of
flash memory.

```c
extern int main(void);
extern uint32_t _estack;
```

`main()` is implemented in `main.c`. `_estack` is defined by the linker script
and points to the top of SRAM. The CPU loads this value into the stack pointer
on reset.

```c
void Reset_Handler(void);
void Default_Handler(void);
```

`Reset_Handler()` is the function executed after reset. `Default_Handler()` is
a fallback interrupt handler, although the current vector table only installs
the reset handler.

```c
__attribute__((section(".isr_vector")))
uint32_t vector_table[] = {
    (uint32_t)&_estack,
    (uint32_t)Reset_Handler,
};
```

This places `vector_table` into the `.isr_vector` section. The linker script
keeps this section at the start of flash. The first entry is the initial stack
pointer. The second entry is the reset handler address.

```c
void Reset_Handler(void) {
    main();

    while (1) {
    }
}
```

After reset, the firmware calls `main()`. If `main()` ever returns, execution
stays trapped in an infinite loop because there is no operating system to
return to.

This startup file is intentionally minimal. A fuller startup file would also
copy initialized `.data` values from flash to SRAM, zero the `.bss` section,
and install handlers for all interrupts.

## main.c

`main.c` configures GPIO Port F and toggles the red LED.

```c
#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define GPIO_PORTF_DIR_R    (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DEN_R    (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))
```

These macros map C names to hardware registers. Each address comes from the
TM4C123 memory map.

- `SYSCTL_RCGCGPIO_R`: enables clocks for GPIO ports.
- `GPIO_PORTF_DIR_R`: selects input or output direction for Port F pins.
- `GPIO_PORTF_DEN_R`: enables digital I/O for Port F pins.
- `GPIO_PORTF_DATA_R`: reads or writes Port F pin values.

The `volatile` keyword is required because these addresses represent hardware.
It prevents the compiler from optimizing away reads and writes that affect the
microcontroller.

```c
#define GPIO_PORTF_CLOCK_EN (1U << 5)
#define RED_LED             (1U << 1)
```

Port F is enabled with bit 5 in the GPIO clock register. The red LED is
connected to PF1, so bit 1 controls it.

```c
static void delay(void) {
    for (volatile uint32_t i = 0; i < 1000000; i++) {
    }
}
```

The delay is a simple busy-wait loop. It wastes CPU cycles to make the LED
blink slowly enough to see. This is fine for a first bare-metal example, but
real firmware usually uses a hardware timer instead.

```c
int main(void) {
    SYSCTL_RCGCGPIO_R |= GPIO_PORTF_CLOCK_EN;
```

The first step in `main()` enables the GPIO Port F peripheral clock. Without
this, writes to the Port F registers would not work correctly.

```c
    volatile uint32_t dummy = SYSCTL_RCGCGPIO_R;
    (void)dummy;
```

This performs a small delay after enabling the clock. Reading the clock
register gives the peripheral time to become ready before configuring Port F.

```c
    GPIO_PORTF_DIR_R |= RED_LED;
    GPIO_PORTF_DEN_R |= RED_LED;
```

These lines configure PF1 as a digital output.

```c
    while (1) {
        GPIO_PORTF_DATA_R ^= RED_LED;
        delay();
    }
}
```

The firmware then runs forever. Each loop toggles PF1 using XOR, then waits.
When PF1 changes between `0` and `1`, the red LED turns off and on.

## Build

From this directory, run:

```sh
make
```

To remove generated files:

```sh
make clean
```

The generated `.elf`, `.bin`, and `.map` files are build artifacts and are
ignored by Git.
