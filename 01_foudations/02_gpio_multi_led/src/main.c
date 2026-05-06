#include <stdint.h>

#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define GPIO_PORTF_DIR_R    (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DEN_R    (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))

#define GPIO_PORTF_CLOCK_EN (1U << 5)
#define RED_LED             (1U << 1)
#define BLUE_LED            (1U << 2)
#define GREEN_LED           (1U << 3)

static void delay(void) {
    for (volatile uint32_t i = 0; i < 1000000; i++) {
    }
}

int main(void) {
    SYSCTL_RCGCGPIO_R |= GPIO_PORTF_CLOCK_EN;

    // Small delay after enabling peripheral clock
    volatile uint32_t dummy = SYSCTL_RCGCGPIO_R;
    (void)dummy;

    GPIO_PORTF_DIR_R |= RED_LED;
    GPIO_PORTF_DIR_R |= BLUE_LED;
    GPIO_PORTF_DIR_R |= GREEN_LED;
    GPIO_PORTF_DEN_R |= RED_LED;
    GPIO_PORTF_DEN_R |= BLUE_LED;
    GPIO_PORTF_DEN_R |= GREEN_LED;
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
}