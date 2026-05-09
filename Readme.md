# Embedded Security Labs

This repository is a hands-on embedded security learning path. The aim is to
build from bare-metal embedded fundamentals toward firmware analysis,
exploitation, secure design, and hardware attack research.

The labs are intentionally practical: each phase should produce code, notes, or
experiments that make the next phase easier to understand. The current work
starts with the Texas Instruments Tiva C Series TM4C123G LaunchPad and focuses
on learning how firmware really runs at the register, memory, linker, and
peripheral level before moving into security topics.

## Roadmap

| Phase | Topics | Tools / Hardware | What You'll Learn |
| --- | --- | --- | --- |
| 1. Foundations | C memory and pointers, ARM Cortex-M, registers, peripherals, UART, SPI, I2C | Texas Instruments Tiva C Series TM4C123G LaunchPad, Code Composer Studio | How embedded systems actually work at a low level |
| 2. Firmware Interaction | Flashing, debugging, memory layout, linker basics | OpenOCD, `arm-none-eabi-gcc` | How firmware is built, loaded, and controlled |
| 3. Intro Security Concepts | Attack surface, debug interfaces such as JTAG/SWD, firmware extraction | Tiva C debugger, `binwalk` | Where vulnerabilities come from in embedded systems |
| 4. Reverse Engineering | Disassembly, function analysis, memory inspection | Ghidra | How to understand unknown firmware binaries |
| 5. Exploitation Basics | Buffer overflows, unsafe C, UART attacks | Python scripts, serial tools | How to break embedded firmware |
| 6. Secure Design | Secure boot, memory protection unit, firmware validation | mbed TLS | How to defend against your own attacks |
| 7. Hardware Attacks Intro | Glitching concepts, side-channel basics | Logic analyzer and oscilloscope, optional | How physical attacks bypass software protections |
| 8. Advanced / Research | Side-channel power analysis, fault injection, secure elements | ChipWhisperer Lite | Real-world hardware exploitation techniques |

## Intended Plan

The repository is intended to grow by phase, not as a separate README rewrite
for every small lab. Each directory should represent a major learning stage, and
the labs inside it should build naturally from simple firmware exercises toward
security-focused experiments.

The planned structure is:

```text
01_foudations/
02_firmware_interaction/
03_intro_security_concepts/
04_reverse_engineering/
05_exploitation_basics/
06_secure_design/
07_hardware_attacks_intro/
08_advanced_research/
```

This top-level README should only change when the overall direction changes in a
meaningful way, such as adding a new phase, changing the target hardware, or
shifting the security focus. Details for individual labs can live inside their
own directories.

## Learning Goal

The end goal is not only to run examples on a microcontroller. The goal is to
understand the complete lifecycle of embedded firmware:

- how firmware is written and linked;
- how it is loaded, debugged, and inspected;
- how implementation mistakes become vulnerabilities;
- how attackers analyze and exploit those mistakes;
- how secure design choices reduce the attack surface.

Each lab should make the system more transparent by connecting source code,
compiler output, memory layout, device registers, and observable board behavior.
