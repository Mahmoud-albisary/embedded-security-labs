PROJECT = uart_echo

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CPU = cortex-m4
CFLAGS = -mcpu=$(CPU) -mthumb -Wall -Wextra -O0 -g -ffreestanding -nostdlib
LDFLAGS = -T linker/tm4c123gh6pm.ld -nostdlib -Wl,-Map=$(PROJECT).map

SRC = src/startup.c src/main.c

all: $(PROJECT).elf $(PROJECT).bin size

$(PROJECT).elf: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $@

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

size: $(PROJECT).elf
	$(SIZE) $<

clean:
	rm -f *.elf *.bin *.map

.PHONY: all clean size