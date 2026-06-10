# GDB, OpenOCD, Flashing, and Breakpoints

These notes explain the debugging and flashing workflow for a TM4C / Cortex-M microcontroller project.

The main goal is to understand what happens after building the code, what `load` means, why OpenOCD and GDB are both used, and how breakpoints behave when the program contains a `while(1)` loop.

---

## 1. Big Picture

When developing firmware for a TM4C microcontroller, the normal workflow looks like this:

```text
Source Code (.c)
       │
       ▼
Compiler
       │
       ▼
Object Files (.o)
       │
       ▼
Linker
       │
       ▼
Executable (.elf)
       │
       ▼
OpenOCD / GDB load
       │
       ▼
TM4C Flash Memory
       │
       ▼
Reset MCU
       │
       ▼
startup.c
       │
       ▼
main()
```

The `.elf` file is the final executable produced by the build process.

It is the file that GDB uses for debugging because it contains not only machine code, but also symbols and debug information.

---

## 2. What Happens During Build?

For example, when you run:

```bash
cmake --build build
```

or press **Build** in the IDE, the compiler translates each `.c` file into an object file:

```text
main.c      → main.o
uart.c      → uart.o
aes.c       → aes.o
startup.c   → startup.o
```

Then the linker combines all object files into one executable:

```text
firmware.elf
```

The ELF file contains:

- machine code
- memory addresses
- function names
- variable names
- debug symbols
- information about Flash and RAM sections

For example, inside the ELF, GDB can know things like:

```text
main            = 0x00001234
UART0_Handler   = 0x00001580
counter         = 0x20000020
```

The microcontroller itself does not understand names like `main` or `counter`.

It only understands addresses and machine instructions.

---

## 3. What is Firmware?

Firmware is software stored inside the microcontroller's Flash memory.

Before flashing:

```text
PC:
    firmware.elf

TM4C Flash:
    old program
```

After flashing:

```text
PC:
    firmware.elf

TM4C Flash:
    new program
```

The program is now physically stored inside the MCU.

That is why it keeps running even after you disconnect and reconnect power.

---

## 4. What Does `load` Mean in GDB?

Inside GDB, you may write:

```gdb
load
```

This means:

```text
Take the firmware from the ELF file on the PC
and write it into the Flash memory of the MCU.
```

GDB itself does not directly talk to the TM4C Flash controller.

Instead, the chain is:

```text
GDB
 │
 │  load
 ▼
OpenOCD
 │
 ▼
Debug Probe / ICDI
 │
 ▼
TM4C Flash
```

So when you type:

```gdb
load
```

GDB reads the ELF file and tells OpenOCD:

```text
Please program these sections into the MCU Flash.
```

OpenOCD then:

1. erases the needed Flash sectors
2. writes the new program bytes
3. verifies the written data
4. usually leaves the CPU halted

After `load`, the firmware is now inside the MCU.

---

## 5. GDB vs OpenOCD

GDB and OpenOCD are not the same thing.

They work together.

---

## 5.1 OpenOCD

OpenOCD talks to the hardware.

```text
PC
 │
 ▼
OpenOCD
 │
 ▼
ICDI / J-Link / ST-Link
 │
 ▼
TM4C
```

OpenOCD knows how to:

- connect to the debugger
- use SWD or JTAG
- reset the MCU
- halt the CPU
- read and write RAM
- read and write Flash
- access CPU registers
- manage hardware breakpoints

OpenOCD is like the hardware translator between your PC and the MCU.

---

## 5.2 GDB

GDB understands the program.

```text
GDB
 │
 ▼
OpenOCD
 │
 ▼
TM4C
```

GDB knows about:

- source files
- functions
- variables
- line numbers
- call stack
- debug symbols

For example, if you write:

```gdb
print counter
```

GDB looks inside the ELF and finds the address of `counter`.

Then it asks OpenOCD to read the memory at that address.

```text
counter → address 0x20000020

GDB asks OpenOCD:
Read memory at 0x20000020
```

OpenOCD reads the RAM from the TM4C and sends the value back to GDB.

---

## 6. Why Does GDB Need the ELF?

When you debug, you usually start GDB like this:

```bash
arm-none-eabi-gdb build/firmware.elf
```

You do not usually debug using only a `.bin` file because a `.bin` file is just raw bytes.

The ELF contains much more information.

For example, when you write:

```gdb
break main
```

The TM4C does not know what `main` means.

GDB looks inside the ELF:

```text
main = 0x00001234
```

Then GDB tells OpenOCD:

```text
Set a breakpoint at address 0x00001234.
```

The MCU only sees an address.

---

## 7. Typical Debugging Session

Usually, you use two terminals.

---

## Terminal 1: Start OpenOCD

```bash
openocd -f board/ek-tm4c123gxl.cfg
```

OpenOCD stays running.

It waits for GDB to connect.

You may see something like:

```text
Listening on port 3333 for gdb connections
Listening on port 4444 for telnet connections
```

---

## Terminal 2: Start GDB

```bash
arm-none-eabi-gdb build/firmware.elf
```

Then inside GDB:

```gdb
target remote localhost:3333
```

Now GDB is connected to OpenOCD.

The chain is:

```text
GDB
 │
 ▼
OpenOCD
 │
 ▼
ICDI / SWD
 │
 ▼
TM4C
```

---

## 8. Common GDB Startup Commands

A common session looks like this:

```gdb
target remote localhost:3333
monitor reset halt
load
break main
continue
```

Meaning:

```text
Connect to OpenOCD
Reset and halt the MCU
Flash the firmware
Set a breakpoint at main
Run until main is reached
```

---

## 9. What is a Breakpoint?

A breakpoint tells the CPU:

```text
Stop when execution reaches this address.
```

For example:

```gdb
break main
```

means:

```text
Stop when the program reaches the beginning of main().
```

The execution flow after reset is usually:

```text
Reset
 ↓
Vector Table
 ↓
Reset_Handler
 ↓
startup.c
 ↓
main()
```

So `break main` stops the CPU here:

```text
Reset
 ↓
startup.c
 ↓
main()  ← CPU stops here
```

This is useful because it proves that:

```text
✓ reset worked
✓ startup.c ran
✓ stack is valid
✓ main() was reached
```

---

## 10. Does `break main` Change My Code?

No.

You do not need to write anything special in your C code.

This:

```gdb
break main
```

does not modify this:

```c
int main(void)
{
    while(1)
    {
    }
}
```

The breakpoint is controlled by the debugger.

---

## 11. What Happens If `main()` Contains `while(1)`?

Most embedded programs contain an infinite loop.

Example:

```c
int main(void)
{
    init_uart();
    init_gpio();

    while(1)
    {
        blink_led();
    }
}
```

If you set:

```gdb
break main
continue
```

execution looks like this:

```text
Reset
 ↓
startup.c
 ↓
main()        ← breakpoint hit here
 ↓
continue
 ↓
init_uart()
 ↓
init_gpio()
 ↓
while(1)
{
    blink_led();
}
```

The CPU stops once at the beginning of `main()`.

After you type:

```gdb
continue
```

the CPU continues into the infinite loop.

---

## 12. Will `break main` Stop Again?

Usually, no.

This is an important point.

The breakpoint at `main` is tied to one address:

```text
main = 0x00001234
```

The CPU reaches that address once:

```text
PC = 0x00001234  ← breakpoint triggers
```

Then after `continue`, the CPU moves forward:

```text
0x00001235
0x00001236
0x00001237
...
```

Eventually it reaches the `while(1)` loop.

It keeps looping there.

It does not go back to the first instruction of `main()`.

So the `main` breakpoint does not trigger again unless:

- you reset the MCU
- the program somehow jumps back to the beginning of `main()`
- you restart the program from reset

---

## 13. Speed Camera Analogy

A breakpoint is like a speed camera on a road.

```text
Road:
----------------X---------------->
                ^
            breakpoint
```

Your program passes this point once.

The breakpoint triggers.

If the program keeps driving forward and never comes back to that exact point, the breakpoint will not trigger again.

For `break main`, the breakpoint is at the entrance of `main()`.

Once you pass it, you are already inside the program.

---

## 14. Breakpoints Inside a Loop

If you want the program to stop repeatedly inside the infinite loop, set a breakpoint inside the loop.

Example:

```c
while(1)
{
    blink_led();
}
```

Set:

```gdb
break blink_led
```

Now execution looks like this:

```text
Loop iteration 1 → blink_led() → breakpoint
Loop iteration 2 → blink_led() → breakpoint
Loop iteration 3 → blink_led() → breakpoint
```

This breakpoint triggers repeatedly because the CPU revisits `blink_led()` every loop iteration.

---

## 15. What If I Have No Breakpoints?

If you do:

```gdb
load
continue
```

and you have no breakpoints, the MCU simply runs.

If your code has:

```c
while(1)
{
}
```

then it runs forever.

GDB will not stop automatically.

To stop it manually, press:

```text
Ctrl + C
```

inside GDB.

This halts the CPU wherever it currently is.

Then you can inspect things:

```gdb
info registers
print counter
backtrace
```

---

## 16. What Are `monitor` Commands?

Commands starting with `monitor` are OpenOCD commands sent through GDB.

Example:

```gdb
monitor reset halt
```

GDB does not execute `reset halt` itself.

Instead, it forwards it to OpenOCD:

```text
GDB
 │
 │  monitor reset halt
 ▼
OpenOCD
 │
 ▼
TM4C
```

So:

```gdb
monitor reset halt
```

means:

```text
GDB, tell OpenOCD to reset and halt the MCU.
```

---

## 17. Common `monitor` Commands

### Reset and Halt

```gdb
monitor reset halt
```

Meaning:

```text
Reset the MCU and stop it immediately.
```

This is useful before loading or debugging.

---

### Reset and Run

```gdb
monitor reset run
```

Meaning:

```text
Reset the MCU and let it run immediately.
```

---

### Halt CPU

```gdb
monitor halt
```

Meaning:

```text
Stop the CPU now.
```

---

### Resume CPU

```gdb
monitor resume
```

Meaning:

```text
Continue running the CPU.
```

Usually, developers use this instead:

```gdb
continue
```

---

### Reset Init

Sometimes you may see:

```gdb
monitor reset init
```

This means:

```text
Reset the MCU and run OpenOCD initialization steps.
```

It can be useful when the target needs special initialization before debugging.

---

## 18. GDB Commands vs Monitor Commands

Normal GDB command:

```gdb
break main
```

Handled mostly by GDB.

GDB uses the ELF to find the address of `main`.

---

Monitor command:

```gdb
monitor reset halt
```

Forwarded to OpenOCD.

OpenOCD resets and halts the hardware.

---

So the difference is:

```text
GDB command:
    Understands program/source-level debugging

monitor command:
    Sends hardware-level command to OpenOCD
```

---

## 19. Useful Commands During Debugging

### Continue Execution

```gdb
continue
```

Resume program execution.

---

### Step Into

```gdb
step
```

Execute one source line.

If the line contains a function call, enter the function.

---

### Step Over

```gdb
next
```

Execute one source line.

If the line contains a function call, run the function without entering it.

---

### Print Variable

```gdb
print counter
```

Print the value of `counter`.

---

### Inspect Registers

```gdb
info registers
```

Show CPU registers.

Important registers include:

```text
PC   = Program Counter
SP   = Stack Pointer
LR   = Link Register
xPSR = Program Status Register
```

---

### Inspect Memory

```gdb
x/16xw 0x20000000
```

Meaning:

```text
Examine 16 words in hexadecimal starting from address 0x20000000.
```

This is useful for inspecting RAM.

---

### Show Call Stack

```gdb
backtrace
```

Shows which functions were called before reaching the current location.

---

## 20. Why `break main` is Useful

`break main` is usually an initial sanity check.

It confirms:

```text
✓ Flashing worked
✓ Reset_Handler executed
✓ startup.c executed
✓ RAM initialization worked enough to reach main
✓ The program did not crash before main
```

After that, you usually debug more specific parts:

```gdb
break uart_send
break UART0_Handler
break aes_encrypt
break main.c:42
```

---

## 21. When Should I Use `break main`?

Use it when:

- starting a debug session
- checking whether startup works
- checking whether the program reaches the application
- debugging early initialization
- testing a newly flashed program

Do not expect it to stop repeatedly during the infinite loop.

For that, put breakpoints inside the loop or inside functions called from the loop.

---

## 22. Complete Mental Model

The full debugging chain is:

```text
GDB
 │
 │ source-level commands
 │ break main
 │ print variable
 │ step
 │ continue
 ▼
OpenOCD
 │
 │ hardware-level commands
 │ reset
 │ halt
 │ flash write
 │ memory access
 ▼
Debug Probe / ICDI
 │
 ▼
TM4C MCU
```

Another way to remember it:

```text
GDB       = debugging brain
OpenOCD   = hardware translator
TM4C      = target MCU
ELF       = map between source code and addresses
```

---

## 23. Typical Workflow Summary

```text
1. Edit code

2. Build project
   cmake --build build

3. Start OpenOCD
   openocd -f board/ek-tm4c123gxl.cfg

4. Start GDB
   arm-none-eabi-gdb build/firmware.elf

5. Connect GDB to OpenOCD
   target remote localhost:3333

6. Reset and halt MCU
   monitor reset halt

7. Load firmware into Flash
   load

8. Set first breakpoint
   break main

9. Run
   continue

10. Debug using:
    next
    step
    print
    info registers
    break function_name
    Ctrl+C
```

---

## 24. Key Takeaways

- Building creates an ELF file on your PC.
- Flashing writes the firmware into the MCU Flash memory.
- `load` in GDB means write the ELF sections into the target memory.
- OpenOCD talks to the hardware.
- GDB understands the program and source code.
- `monitor` commands are OpenOCD commands sent through GDB.
- `break main` stops once when the program first reaches `main()`.
- If `main()` contains `while(1)`, the CPU will continue looping after you type `continue`.
- `break main` will not stop again unless the MCU resets or execution returns to the beginning of `main()`.
- To stop repeatedly inside a loop, set a breakpoint inside the loop or inside a function called by the loop.
- If the firmware is running forever, press `Ctrl+C` in GDB to halt the CPU manually.
