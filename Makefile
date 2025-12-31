ASM = nasm
CC = gcc
LD = ld

CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -Wall -Wextra -c
LDFLAGS = -m elf_i386 -T linker.ld

KERNEL_SOURCES = $(wildcard kernel/*.c) $(wildcard drivers/*.c) $(wildcard fs/*.c) $(wildcard installer/*.c) $(wildcard shell/*.c) $(wildcard bin/*.c)
KERNEL_OBJECTS = $(KERNEL_SOURCES:.c=.o)

all: EphemeralOS.img

boot/boot.bin: boot/boot.asm
	$(ASM) -f bin boot/boot.asm -o boot/boot.bin

boot/stage2.bin: boot/stage2.asm
	$(ASM) -f bin boot/stage2.asm -o boot/stage2.bin

# Agregar regla para compilar entry.asm
kernel/entry.o: kernel/entry.asm
	$(ASM) -f elf32 kernel/entry.asm -o kernel/entry.o

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# Modificar para incluir entry.o al principio
kernel.bin: kernel/entry.o $(KERNEL_OBJECTS)
	$(LD) $(LDFLAGS) -o kernel.bin kernel/entry.o $(KERNEL_OBJECTS)

EphemeralOS.img: boot/boot.bin boot/stage2.bin kernel.bin
	cat boot/boot.bin boot/stage2.bin kernel.bin > temp.img
	dd if=temp.img of=EphemeralOS.img bs=512 count=2880
	dd if=/dev/zero of=EphemeralOS.img bs=512 seek=2880 count=0
	rm -f temp.img

clean:
	rm -f boot/*.bin kernel/entry.o $(KERNEL_OBJECTS) kernel.bin EphemeralOS.img temp.img

run: EphemeralOS.img
	qemu-system-i386 -fda EphemeralOS.img

.PHONY: all clean run