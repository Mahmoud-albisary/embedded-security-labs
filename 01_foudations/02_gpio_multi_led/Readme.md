# GPIO Multi LED

In this subproject, the goal is to switch between multiple LED colors instead
of blinking only one LED as in the previous project.

The linker script, `startup.c`, and `Makefile` are essentially the same as in
the first blink project. The main difference is the project name in the
Makefile and the GPIO logic in `main.c`.

## What This Project Does

The TM4C123 LaunchPad has an RGB LED connected to GPIO Port F:

- PF1 controls the red LED.
- PF2 controls the blue LED.
- PF3 controls the green LED.

This project enables all three LED pins as outputs, then turns each color on
and off in sequence:

1. Turn on red.
2. Wait.
3. Turn off red and turn on blue.
4. Wait.
5. Turn off blue and turn on green.
6. Wait.
7. Turn off green.
8. Repeat forever.

## Makefile

The Makefile builds the firmware in the same way as the previous bare-metal
blink project.

```make
PROJECT = gpio_multi_led
```

This changes the generated output names to:

- `gpio_multi_led.elf`
- `gpio_multi_led.bin`
- `gpio_multi_led.map`

The source files are still:

```make
SRC = src/startup.c src/main.c
```

`startup.c` provides the reset handler and vector table. `main.c` contains the
GPIO application code.

The default target:

```make
all: $(PROJECT).elf $(PROJECT).bin size
```

builds the ELF file, converts it to a raw binary, and prints the firmware size.

## main.c

The register definitions are the same style used in the first project:

```c
#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define GPIO_PORTF_DIR_R    (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DEN_R    (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))
```

Each macro maps a C name to a memory-mapped hardware register:

- `SYSCTL_RCGCGPIO_R` enables the GPIO peripheral clock.
- `GPIO_PORTF_DIR_R` configures Port F pins as inputs or outputs.
- `GPIO_PORTF_DEN_R` enables digital function on Port F pins.
- `GPIO_PORTF_DATA_R` reads or writes Port F pin values.

The LED pins are defined as bit masks:

```c
#define RED_LED             (1U << 1)
#define BLUE_LED            (1U << 2)
#define GREEN_LED           (1U << 3)
```

These correspond to PF1, PF2, and PF3.

## GPIO Setup

First, the firmware enables the Port F clock:

```c
SYSCTL_RCGCGPIO_R |= GPIO_PORTF_CLOCK_EN;
```

After enabling the clock, the code reads the clock register once:

```c
volatile uint32_t dummy = SYSCTL_RCGCGPIO_R;
(void)dummy;
```

This gives the peripheral a short time to become ready before the code accesses
the GPIO registers.

Then the three LED pins are configured as outputs:

```c
GPIO_PORTF_DIR_R |= RED_LED;
GPIO_PORTF_DIR_R |= BLUE_LED;
GPIO_PORTF_DIR_R |= GREEN_LED;
```

Finally, digital I/O is enabled for the same pins:

```c
GPIO_PORTF_DEN_R |= RED_LED;
GPIO_PORTF_DEN_R |= BLUE_LED;
GPIO_PORTF_DEN_R |= GREEN_LED;
```

Without enabling digital I/O, the pins would not behave as normal GPIO outputs.

## LED Sequence

The main loop runs forever:

```c
while (1) {
    GPIO_PORTF_DATA_R ^= RED_LED;
    delay();
    GPIO_PORTF_DATA_R ^= RED_LED;
    GPIO_PORTF_DATA_R ^= BLUE_LED;
    delay();
    GPIO_PORTF_DATA_R ^= BLUE_LED;
    GPIO_PORTF_DATA_R ^= GREEN_LED;
    delay();
    GPIO_PORTF_DATA_R ^= GREEN_LED;
}
```

The `^=` operator toggles a bit:

- If the bit is `0`, it becomes `1`.
- If the bit is `1`, it becomes `0`.

So this line turns the red LED on the first time it runs:

```c
GPIO_PORTF_DATA_R ^= RED_LED;
```

After the delay, the same operation turns the red LED off:

```c
GPIO_PORTF_DATA_R ^= RED_LED;
```

The code then toggles the blue LED on, waits, toggles it off, toggles the green
LED on, waits, and toggles it off.

## Delay

The delay function is a simple busy-wait loop:

```c
static void delay(void) {
    for (volatile uint32_t i = 0; i < 1000000; i++) {
    }
}
```

The `volatile` keyword prevents the compiler from removing the empty loop. This
is useful for a simple learning project, but real firmware usually uses a timer
peripheral instead of wasting CPU cycles in a busy-wait.

## Build

From this directory, run:

```sh
make
```

To remove generated build files:

```sh
make clean
```
