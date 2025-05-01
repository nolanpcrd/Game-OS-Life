TOOLCHAIN_PREFIX = aarch64-elf-
AS = $(TOOLCHAIN_PREFIX)as
GCC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy

CPU = cortex-a53

ASM_SOURCES = boot.s
C_SOURCES = kernel.c libs/screen.c libs/game.c
OBJECTS = $(ASM_SOURCES:.s=.o) $(C_SOURCES:.c=.o)

KERNEL_ELF = kernel.elf
KERNEL_IMG = kernel.img

CFLAGS = -ffreestanding -nostdlib -O2 -Wall -Wextra -g -march=armv8-a -mcpu=$(CPU)
AFLAGS = -g
LDFLAGS = -T linker.ld

run: $(KERNEL_IMG)
	qemu-system-aarch64 -machine raspi3b -cpu $(CPU) -kernel $(KERNEL_IMG) -serial stdio

clean:
	rm -f $(OBJECTS) $(KERNEL_ELF) $(KERNEL_IMG)

$(KERNEL_IMG): $(KERNEL_ELF)
	$(OBJCOPY) $(KERNEL_ELF) -O binary $(KERNEL_IMG)

$(KERNEL_ELF): $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) $(OBJECTS) -o $(KERNEL_ELF)


%.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@


%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

.PHONY: run clean