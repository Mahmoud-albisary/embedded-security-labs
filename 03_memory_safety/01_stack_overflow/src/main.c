#include <stdint.h>

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

// Do not run this function, as it will cause a stack overflow due to infinite recursion and may cause damage to the RAM. 
// Uncommenting this function and calling it will lead to a stack overflow, which can cause the program to crash or behave unpredictably.
// void bar() {
//     volatile uint32_t c = 0xCCCCCCCC;
//     bar(); // This will cause a stack overflow due to infinite recursion
// }
int main() {
    foo();
    //bar();
    return 0;
}