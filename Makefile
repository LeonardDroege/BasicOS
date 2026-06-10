PROJECT_ROOT = $(shell pwd)
BUILD_DIR = $(PROJECT_ROOT)/source/kernel/build
ISO_DIR = $(PROJECT_ROOT)/source/kernel/iso
BOOT_DIR = $(ISO_DIR)/boot

.PHONY: all clean run

all:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/kernel.bin
	rm -f $(BOOT_DIR)/kernel.bin
	rm -f $(ISO_DIR)/kernel.iso
	rm -f serial.log
	
	@echo "Assembling header.asm..."
	nasm -f elf64 source/kernel/asm/header.asm -o source/kernel/build/header.o
	
	@echo "Assembling main.asm..."
	nasm -f elf64 source/kernel/asm/main.asm -o source/kernel/build/main.o
	
	@echo "Assembling main64.asm..."
	nasm -f elf64 source/kernel/asm/main64.asm -o source/kernel/build/main64.o
	
	@echo "Assembling interrupts.asm..."
	nasm -f elf64 source/kernel/asm/interrupts.asm -o source/kernel/build/interrupts.o
	
	@echo "Compiling main.c..."
	x86_64-linux-gnu-gcc -c -I source/kernel/intf -ffreestanding source/kernel/c/main.c -o source/kernel/build/main_c.o
	
	@echo "Linking kernel..."
	x86_64-linux-gnu-ld -n -o source/kernel/build/kernel.bin -T source/kernel/linker.ld source/kernel/build/header.o source/kernel/build/main.o source/kernel/build/main64.o source/kernel/build/main_c.o source/kernel/build/interrupts.o
	
	@echo "Copying kernel to ISO directory..."
	cp source/kernel/build/kernel.bin source/kernel/iso/boot/kernel.bin
	
	@echo "Creating ISO (removing old file first)..."
	rm -f source/kernel/iso/kernel.iso
	grub-mkrescue -o source/kernel/iso/kernel.iso source/kernel/iso
	
	@echo "Running in QEMU..."
	qemu-system-x86_64 -cdrom source/kernel/iso/kernel.iso -serial file:serial.log

clean:
	@echo "Cleaning all build artifacts..."
	rm -rf source/kernel/build/*.o source/kernel/build/kernel.bin
	rm -f source/kernel/iso/boot/kernel.bin
	rm -f source/kernel/iso/kernel.iso
	@echo "Clean complete"

run: all

# For when you only want to build without running
build:
	rm -rf source/kernel/build/*.o source/kernel/build/kernel.bin
	rm -f source/kernel/iso/boot/kernel.bin
	rm -f source/kernel/iso/kernel.iso
	nasm -f elf64 source/kernel/asm/header.asm -o source/kernel/build/header.o
	nasm -f elf64 source/kernel/asm/main.asm -o source/kernel/build/main.o
	nasm -f elf64 source/kernel/asm/main64.asm -o source/kernel/build/main64.o
	nasm -f elf64 source/kernel/asm/interrupts.asm -o source/kernel/build/interrupts.o
	x86_64-linux-gnu-gcc -c -I source/kernel/intf -ffreestanding source/kernel/c/main.c -o source/kernel/build/main_c.o
	x86_64-linux-gnu-ld -n -o source/kernel/build/kernel.bin -T source/kernel/linker.ld source/kernel/build/header.o source/kernel/build/main.o source/kernel/build/main64.o source/kernel/build/main_c.o source/kernel/build/interrupts.o
	cp source/kernel/build/kernel.bin source/kernel/iso/boot/kernel.bin
	rm -f source/kernel/iso/kernel.iso
	grub-mkrescue -o source/kernel/iso/kernel.iso source/kernel/iso

# For running without rebuilding
run:
	qemu-system-x86_64 -cdrom source/kernel/iso/kernel.iso
