#include <stdint.h>

#define SYSCTL_RCGCGPIO_R   (*((volatile uint32_t *)0x400FE608))
#define GPIO_PORTF_DIR_R    (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DEN_R    (*((volatile uint32_t *)0x4002551C))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))
#define GPIO_PORTF_PUR_R    (*((volatile uint32_t *)0x40025510))
#define GPIO_PORTF_LOCK_R   (*((volatile uint32_t *)0x40025520))
#define GPIO_PORTF_CR_R     (*((volatile uint32_t *)0x40025524))

#define GPIO_PORTF_CLOCK_EN (1U << 5)
#define GPIO_PORTF_LOCK_KEY 0x4C4F434B
#define SW1_BUTTON          (1U << 4)
#define SW2_BUTTON          (1U << 0)
#define RED_LED             (1U << 1)
#define BLUE_LED            (1U << 2)
#define GREEN_LED           (1U << 3)
#define PURPLE_LED          (RED_LED | BLUE_LED)
#define YELLOW_LED          (RED_LED | GREEN_LED)
#define CYAN_LED            (BLUE_LED | GREEN_LED)
#define WHITE_LED           (RED_LED | BLUE_LED | GREEN_LED)
#define NUM_LEDS            7


const uint32_t leds[] = {
    RED_LED,
    BLUE_LED,
    GREEN_LED,
    PURPLE_LED,
    YELLOW_LED,
    CYAN_LED,
    WHITE_LED
};

void debounce_delay(void) {
    for (volatile uint32_t i = 0; i < 50000; i++) {
    }
}
int main(void) {
    SYSCTL_RCGCGPIO_R |= GPIO_PORTF_CLOCK_EN;
    
    volatile uint32_t dummy = SYSCTL_RCGCGPIO_R;
    // Small delay after enabling peripheral clock
    (void)dummy;
    
    GPIO_PORTF_LOCK_R = GPIO_PORTF_LOCK_KEY;
    GPIO_PORTF_CR_R |= SW2_BUTTON;
    GPIO_PORTF_PUR_R |= SW1_BUTTON | SW2_BUTTON;

    GPIO_PORTF_DIR_R |= RED_LED;
    GPIO_PORTF_DIR_R |= BLUE_LED;
    GPIO_PORTF_DIR_R |= GREEN_LED;
    GPIO_PORTF_DEN_R |= RED_LED;
    GPIO_PORTF_DEN_R |= BLUE_LED;
    GPIO_PORTF_DEN_R |= GREEN_LED;
    GPIO_PORTF_DIR_R &= ~SW1_BUTTON;
    GPIO_PORTF_DIR_R &= ~SW2_BUTTON;
    GPIO_PORTF_DEN_R |= SW1_BUTTON;
    GPIO_PORTF_DEN_R |= SW2_BUTTON;
    uint8_t i = 0;
    int sw2_pressed = 0;
    int prev_sw2_state = 0; // Not pressed
    while (1) {

        if(!(GPIO_PORTF_DATA_R & SW1_BUTTON)) {
            GPIO_PORTF_DATA_R = (GPIO_PORTF_DATA_R & ~WHITE_LED) | leds[i];
        } else {
                GPIO_PORTF_DATA_R = (GPIO_PORTF_DATA_R & ~WHITE_LED);
        }
        
        sw2_pressed = !(GPIO_PORTF_DATA_R & SW2_BUTTON);
        if (sw2_pressed && !prev_sw2_state) {
            debounce_delay();
            if (!(GPIO_PORTF_DATA_R & SW2_BUTTON)) {
                i++;
                if (i >= NUM_LEDS) {
                    i = 0;
                }  
            }
        }
        prev_sw2_state = sw2_pressed;
    }
}