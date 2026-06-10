# TM4C123 Memory Layout and Startup Process

## Overview

One of the most important concepts in embedded systems is understanding where data is stored.

Many beginners hear terms such as:

- Flash
- RAM
- Stack
- Heap
- .data
- .bss
- Peripheral Registers

and think they are separate memories.

In reality, most of them are simply different regions inside the same SRAM.

---

# Complete MCU Memory Map

```text
+------------------------------------------------+
|                  FLASH                          |
|------------------------------------------------|
| Startup Code                                   |
| Program Code (.text)                           |
| Constants (.rodata)                            |
| Initial values for .data                       |
+------------------------------------------------+

+------------------------------------------------+
|                   SRAM                          |
|------------------------------------------------|
| .data                                          |
| .bss                                           |
| Heap                                           |
| Free RAM                                       |
| Stack                                          |
+------------------------------------------------+

+------------------------------------------------+
|          Peripheral Registers                   |
|------------------------------------------------|
| GPIO                                           |
| UART                                           |
| ADC                                            |
| Timers                                         |
| PWM                                            |
| I2C                                            |
| DMA                                            |
+------------------------------------------------+
```

---

# Flash Memory

Flash is the permanent storage of the MCU.

Think of it as the SSD of a computer.

TM4C123:

```text
Flash Size = 256 KB
```

Contents:

```c
const char msg[] = "Hello";

int global = 5;

int main()
{
    ...
}
```

Before the MCU starts running, all of this exists only in Flash.

---

## Characteristics

### Advantages

```text
✓ Non-volatile
✓ Large
✓ Keeps contents after power loss
```

### Disadvantages

```text
✗ Slower than SRAM
✗ Limited write cycles
```

---

# SRAM

SRAM is the MCU's working memory.

Think of it as the RAM of a PC.

TM4C123:

```text
SRAM Size = 32 KB
```

All runtime data is stored here.

---

# SRAM Layout

Typical layout:

```text
High Address
0x20008000
+----------------------+
| Stack                |
|                      |
| grows downward       |
+----------------------+
|                      |
| Free RAM             |
|                      |
+----------------------+
| Heap                 |
| grows upward         |
+----------------------+
| .bss                 |
+----------------------+
| .data                |
+----------------------+
0x20000000
Low Address
```

---

# The .data Section

Contains:

```text
Initialized global variables
Initialized static variables
```

Example:

```c
int counter = 5;

static int speed = 100;
```

Both belong to `.data`.

---

## Why?

Because they already have values.

The compiler must remember:

```text
counter = 5
speed = 100
```

These values are first stored in Flash.

During startup:

```text
Flash ---> SRAM
```

The startup code copies them into RAM.

---

## Example

Before reset:

```text
FLASH
-----------------
counter = 5
speed   = 100
```

After Reset_Handler:

```text
SRAM (.data)
-----------------
counter = 5
speed   = 100
```

---

# The .bss Section

Contains:

```text
Uninitialized global variables
Uninitialized static variables
```

Example:

```c
int temperature;

static int state;
```

No values are provided.

---

## Why not leave them random?

The C standard requires:

```text
All uninitialized globals start at 0
```

Therefore startup.c clears .bss.

Equivalent to:

```c
temperature = 0;
state = 0;
```

---

## Example

Before startup:

```text
SRAM
-----------------
???
???
???
```

After startup:

```text
SRAM (.bss)
-----------------
temperature = 0
state       = 0
```

---

# Stack

The stack stores temporary data.

Every function call uses the stack.

Example:

```c
void foo()
{
    int x = 10;
}
```

When foo() is called:

```text
Stack
-----------------
x
return address
saved registers
```

When foo() returns:

```text
Stack
-----------------
removed
```

---

# What is stored on the Stack?

Typical contents:

```text
Local variables
Function parameters
Return addresses
Saved CPU registers
Interrupt context
```

---

## Example

```c
int add(int a, int b)
{
    int result = a + b;
    return result;
}
```

Stack frame:

```text
+----------------+
| result         |
+----------------+
| parameter b    |
+----------------+
| parameter a    |
+----------------+
| return address |
+----------------+
```

---

# Why Use the Stack?

Imagine:

```c
void foo()
{
    int x;
}
```

If x were global:

```c
int x;
```

then every function would permanently consume memory.

The stack allows temporary allocation.

Memory is automatically reclaimed when the function exits.

---

# Heap

Heap is another RAM region.

Used for dynamic allocation.

Example:

```c
char *buffer = malloc(100);
```

Memory comes from the heap.

---

# Heap Growth

```text
Low Address
+----------------+
| .data          |
+----------------+
| .bss           |
+----------------+
| Heap           |
|      ↑         |
+----------------+
| Free RAM       |
+----------------+
|      ↓         |
| Stack          |
+----------------+
High Address
```

---

# Why Embedded Systems Often Avoid Heap

Problems:

```text
Memory fragmentation
Unpredictable timing
Allocation failures
Harder debugging
```

Many embedded projects never call:

```c
malloc()
free()
```

---

# Global vs Local Variables

Example:

```c
int global_var = 5;

int main()
{
    int local_var = 10;
}
```

Memory layout:

```text
.data
-----------------
global_var

Stack
-----------------
local_var
```

Both are in SRAM.

Only their locations differ.

---

# Where Are Constants Stored?

Example:

```c
const char text[] = "UART";
```

Stored in:

```text
Flash (.rodata)
```

Reason:

```text
Never changes
No need to waste RAM
```

---

# Peripheral Registers

Peripheral registers are NOT RAM.

Example:

```c
GPIO_PORTF_DATA_R = 0x02;
```

This writes directly to hardware.

---

## Memory-Mapped Hardware

```text
CPU
 |
 | Bus
 |
 +----------------+
 | GPIO Peripheral|
 +----------------+
```

Register address example:

```text
GPIO Port F Data Register

0x40025000
```

---

# UART Peripheral

UART contains its own registers.

Typical UART hardware:

```text
+----------------------+
| UART                 |
|----------------------|
| Control Register     |
| Status Register      |
| Baud Rate Register   |
| TX FIFO              |
| RX FIFO              |
+----------------------+
```

---

# UART FIFOs

UART has small internal storage.

Example:

```text
TX FIFO = 16 bytes
RX FIFO = 16 bytes
```

Data written to UART first enters:

```text
TX FIFO
```

before transmission.

---

# Timer Peripheral

Timer hardware contains:

```text
Counter Register
Prescaler Register
Compare Register
Status Register
```

Example:

```c
TIMER0_TAILR_R = 16000;
```

This configures hardware.

Not RAM.

---

# ADC Peripheral

ADC contains:

```text
Control Registers
Status Registers
Sample FIFOs
```

Converted samples are temporarily stored in ADC FIFOs.

---

# DMA Controller

DMA contains:

```text
Channel Configuration
Status Registers
Transfer Control
Address Registers
```

DMA itself does not hold much data.

Instead it moves data between:

```text
Peripheral <-> SRAM
Peripheral <-> Peripheral
SRAM <-> SRAM
```

without CPU intervention.

---

# Startup Process

When power is applied:

```text
1. CPU loads Stack Pointer
2. CPU jumps to Reset_Handler
3. .data copied Flash -> SRAM
4. .bss cleared to zero
5. System initialization
6. main()
```

---

# Visual Startup Example

Before startup:

```text
FLASH
--------------------
counter = 5

SRAM
--------------------
random values
```

After startup:

```text
FLASH
--------------------
counter = 5

SRAM (.data)
--------------------
counter = 5

SRAM (.bss)
--------------------
all zeros
```

---

# Important Takeaway

Most beginners think:

```text
RAM
Stack
Heap
```

are separate memories.

They are not.

The correct picture is:

```text
SRAM
|
+-- .data
|
+-- .bss
|
+-- Heap
|
+-- Stack
```

All of them share the same physical RAM.

The only difference is:

- What type of data is stored there
- Who manages it
- How long the data lives