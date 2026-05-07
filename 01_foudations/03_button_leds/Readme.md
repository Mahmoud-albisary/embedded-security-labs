# Button LEDs

In this subproject, the goal is to control the RGB LED using the two onboard
buttons on the TM4C123 LaunchPad.

The linker script, `startup.c`, and `Makefile` are the same as in the previous
projects. The only Makefile difference is the project name:

```make
PROJECT = button_leds
```

The important changes are in `main.c`.

## What This Project Does

The board uses GPIO Port F for both the RGB LED and the onboard switches:

- PF0 is SW2.
- PF1 is the red LED.
- PF2 is the blue LED.
- PF3 is the green LED.
- PF4 is SW1.

The firmware keeps a list of seven LED colors:

1. Red
2. Blue
3. Green
4. Purple
5. Yellow
6. Cyan
7. White

SW1 displays the currently selected color while it is pressed. When SW1 is
released, the LED turns off.

SW2 moves to the next color in the list. When the last color is reached, the
index wraps back to the first color.

## Register Definitions

The project uses direct memory-mapped register access:

```c
#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define GPIO_PORTF_DIR_R    (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DEN_R    (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))
#define GPIO_PORTF_PUR_R    (*((volatile uint32_t *)0x40025510))
#define GPIO_PORTF_LOCK_R   (*((volatile uint32_t *)0x40025520))
#define GPIO_PORTF_CR_R     (*((volatile uint32_t *)0x40025524))
```

These names represent hardware registers in GPIO Port F:

- `SYSCTL_RCGCGPIO_R` enables the GPIO peripheral clock.
- `GPIO_PORTF_DIR_R` configures pins as inputs or outputs.
- `GPIO_PORTF_DEN_R` enables digital I/O.
- `GPIO_PORTF_DATA_R` reads button states and writes LED states.
- `GPIO_PORTF_PUR_R` enables internal pull-up resistors.
- `GPIO_PORTF_LOCK_R` unlocks protected GPIO pins.
- `GPIO_PORTF_CR_R` allows changes to protected pins after unlocking.

## LEDs and Buttons

The LED and button pins are defined as bit masks:

```c
#define SW1_BUTTON          (1U << 4)
#define SW2_BUTTON          (1U << 0)
#define RED_LED             (1U << 1)
#define BLUE_LED            (1U << 2)
#define GREEN_LED           (1U << 3)
```

The RGB LED can display more colors by combining red, blue, and green:

```c
#define PURPLE_LED          (RED_LED | BLUE_LED)
#define YELLOW_LED          (RED_LED | GREEN_LED)
#define CYAN_LED            (BLUE_LED | GREEN_LED)
#define WHITE_LED           (RED_LED | BLUE_LED | GREEN_LED)
```

The color list is stored in an array:

```c
const uint32_t leds[] = {
    RED_LED,
    BLUE_LED,
    GREEN_LED,
    PURPLE_LED,
    YELLOW_LED,
    CYAN_LED,
    WHITE_LED
};
```

The variable `i` is used as the current color index.

## GPIO Setup

First, the firmware enables the Port F clock:

```c
SYSCTL_RCGCGPIO_R |= GPIO_PORTF_CLOCK_EN;
```

Then it reads the clock register once:

```c
volatile uint32_t dummy = SYSCTL_RCGCGPIO_R;
(void)dummy;
```

This gives the GPIO peripheral a short time to become ready.

## Unlocking SW2

PF0 is locked by default on the TM4C123 because it can be used for a special
function. Since SW2 is connected to PF0, the code must unlock it before using
it as a normal GPIO input:

```c
GPIO_PORTF_LOCK_R = GPIO_PORTF_LOCK_KEY;
GPIO_PORTF_CR_R |= SW2_BUTTON;
```

`GPIO_PORTF_LOCK_KEY` is the required unlock value:

```c
#define GPIO_PORTF_LOCK_KEY 0x4C4F434B
```

After this, PF0 can be configured like a normal GPIO pin.

## Pull-Up Resistors

The onboard switches are active-low. That means:

- Not pressed reads as `1`.
- Pressed reads as `0`.

The code enables internal pull-up resistors for SW1 and SW2:

```c
GPIO_PORTF_PUR_R |= SW1_BUTTON | SW2_BUTTON;
```

Because of this, button checks use `!`:

```c
if (!(GPIO_PORTF_DATA_R & SW1_BUTTON)) {
```

This condition is true when SW1 is pressed.

## LED and Button Direction

The RGB LED pins are configured as outputs:

```c
GPIO_PORTF_DIR_R |= RED_LED;
GPIO_PORTF_DIR_R |= BLUE_LED;
GPIO_PORTF_DIR_R |= GREEN_LED;
```

The button pins are configured as inputs:

```c
GPIO_PORTF_DIR_R &= ~SW1_BUTTON;
GPIO_PORTF_DIR_R &= ~SW2_BUTTON;
```

Digital I/O is then enabled for both LEDs and buttons:

```c
GPIO_PORTF_DEN_R |= RED_LED;
GPIO_PORTF_DEN_R |= BLUE_LED;
GPIO_PORTF_DEN_R |= GREEN_LED;
GPIO_PORTF_DEN_R |= SW1_BUTTON;
GPIO_PORTF_DEN_R |= SW2_BUTTON;
```

## SW1 Logic

SW1 controls whether the selected color is shown:

```c
if (!(GPIO_PORTF_DATA_R & SW1_BUTTON)) {
    GPIO_PORTF_DATA_R = (GPIO_PORTF_DATA_R & ~WHITE_LED) | leds[i];
} else {
    GPIO_PORTF_DATA_R = (GPIO_PORTF_DATA_R & ~WHITE_LED);
}
```

When SW1 is pressed, the code clears the RGB LED bits and then writes the
selected color from `leds[i]`.

When SW1 is not pressed, the code clears the RGB LED bits, turning the LED off.

`WHITE_LED` is used as a mask because it contains all three LED bits:

```c
#define WHITE_LED           (RED_LED | BLUE_LED | GREEN_LED)
```

So this expression clears only the RGB LED pins:

```c
GPIO_PORTF_DATA_R & ~WHITE_LED
```

## SW2 Logic

SW2 changes the selected color:

```c
sw2_pressed = !(GPIO_PORTF_DATA_R & SW2_BUTTON);
```

Since SW2 is active-low, this variable becomes `1` when SW2 is pressed.

The code uses edge detection:

```c
if (sw2_pressed && !prev_sw2_state) {
```

This condition is true only when SW2 changes from not pressed to pressed. That
prevents the color index from increasing continuously while the button is held.

After detecting a new press, the code waits briefly:

```c
debounce_delay();
```

Then it checks SW2 again to make sure the button is still pressed:

```c
if (!(GPIO_PORTF_DATA_R & SW2_BUTTON)) {
```

If the press is confirmed, the color index is incremented:

```c
i++;
if (i >= NUM_LEDS) {
    i = 0;
}
```

This wraps the color index back to `0` after the last color.

Finally, the current SW2 state is saved:

```c
prev_sw2_state = sw2_pressed;
```

This saved state is what makes edge detection possible on the next loop.

## Debounce Delay

Mechanical buttons can bounce. A single physical press may briefly look like
many fast presses to the microcontroller.

This project uses a simple software delay:

```c
void debounce_delay(void) {
    for (volatile uint32_t i = 0; i < 50000; i++) {
    }
}
```

The delay gives the button signal time to settle before the code accepts the
press as real. This is a simple blocking debounce approach, which is fine for
this learning project.

## Build

From this directory, run:

```sh
make
```

To remove generated build files:

```sh
make clean
```
