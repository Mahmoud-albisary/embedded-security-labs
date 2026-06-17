PROJECT = stack_overflow

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = cortex-m4
SRC = src/startup.c src/main.c

COMMON_CFLAGS = -mcpu=$(CPU) -mthumb -Wall -Wextra -O0 -g3 -ffreestanding -nostdlib
LDFLAGS = -T linker/tm4c123gh6pm.ld -nostdlib -Wl,-Map=$(PROJECT).map

all: vulnerable protected

vulnerable: PROJECT = stack_overflow_vulnerable
vulnerable: CFLAGS_EXTRA = -fno-stack-protector
vulnerable: $(PROJECT)_vulnerable.elf $(PROJECT)_vulnerable.bin size

protected: PROJECT = stack_overflow_protected
protected: CFLAGS_EXTRA = -fstack-protector-all
protected: $(PROJECT)_protected.elf $(PROJECT)_protected.bin size

%.elf: $(SRC)
	$(CC) $(COMMON_CFLAGS) $(CFLAGS_EXTRA) $(SRC) $(LDFLAGS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

size: $(PROJECT).elf
	$(SIZE) $<

clean:
	rm -f *.elf *.bin *.map

.PHONY: all vulnerable protected clean size