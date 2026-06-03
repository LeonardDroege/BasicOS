#include <stdbool.h>
#include <stddef.h>
#include "memory.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "terminal.h"

extern uint32_t mb_magic;

void print_memory_map()
{
	for(size_t i = 0; i < usable_count; i++)
	{
		terminal_writestring("[Usable] ", 0);
		terminal_writestring(to_hex64(usable_memory[i].base), 0);
		terminal_writestring(" - ", 0);
		terminal_writestring(to_hex64(usable_memory[i].length + usable_memory[i].base), 0);
		terminal_writestring(" ", 0);

		uint64_t size = usable_memory[i].length / 1024;
		char isMb = size > 1024 ? 1 : 0;
		size = isMb == 1 ? size / 1024 : size;
		terminal_writestring("(", 0);
		terminal_writestring(to_dec(size), 0);
		terminal_writestring(isMb == 1 ?"MB)\n" : "KB)\n", 0);
	}

    terminal_writestring("\n", 0);
}

void kheap_dump(void) {
	terminal_writestring("\n", 0);
    terminal_writestring("=== Kernel Heap Dump ===\n", 0);

    kheap_block_t* curr = kheap_head;
    int index = 0;

    while (curr) {
        terminal_writestring("Block ", 0);
        terminal_writestring(to_dec(index++), 0);
        terminal_writestring(": ", 0);

        terminal_writestring(curr->free ? "[FREE] " : "[USED] ", 0);

        terminal_writestring("Addr=", 0);
        terminal_writestring(to_hex32((uint32_t)curr), 0);

        terminal_writestring(" Size=", 0);
        terminal_writestring(to_dec((uint64_t)curr->size), 0);

        terminal_writestring(" Next=", 0);
        terminal_writestring(to_hex32((uint32_t)curr->next), 0);

        terminal_writestring("\n", 0);

        curr = curr->next;
    }

    terminal_writestring("========================\n\n", 0);
	terminal_writestring("", 1);
}

void isr0_handler()
{
    terminal_writestring("Divide by zero!\n", 0);
    for(;;);
}

#define COM1_PORT 0x3F8

void setup_com_port(void)
{
	outb(COM1_PORT + 1, 0x00);
	outb(COM1_PORT + 3, 0x80);
	outb(COM1_PORT + 0, 0x03);
	outb(COM1_PORT + 1, 0x00);
	outb(COM1_PORT + 3, 0x03);
	outb(COM1_PORT + 2, 0xC7);
	outb(COM1_PORT + 4, 0x0B);
	outb(COM1_PORT + 4, 0x1E);
	outb(COM1_PORT + 0, 0xAE);

	if(inb(COM1_PORT + 0) != 0xAE)
	{
		return;
	}

	outb(COM1_PORT + 4, 0x0F);
}

int is_transmit_empty() {
   return inb(COM1_PORT + 5) & 0x20;
}

void write_to_com_port(char c)
{
	while(is_transmit_empty() == 0);
	outb(COM1_PORT, c);
}

extern uint64_t framebuffer_addr;

void kernel_main()
{
    terminal_initialize();
	
	terminal_writestring("BasicOS\n\n", 0);

    if(mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        terminal_writestring("Bad multiboot2 magic!\n\n", 0);
        terminal_writestring(to_hex32(mb_magic), 0);
		return;
	}
	terminal_writestring("Initialized multiboot2!\n\n", 0);

	setup_com_port();

    parse_memory_map();
    print_memory_map();

    pmm_init();
    kheap_init();

    terminal_writestring("Initialized pmm and heap!\n\n", 0);

	idt_init();
	pic_remap();

	uint8_t mask = inb(0x21);
	mask |= (1 << 0);
	mask &= ~(1 << 1);
	outb(0x21, mask);

	keyboard_init();

	__asm__ volatile ("sti");

	terminal_writestring("GDT base: ", 0);
	terminal_writestring(to_hex64(gdt_ptr.base), 0);
	terminal_writestring(" limit: ", 0);
	terminal_writestring(to_hex32(gdt_ptr.limit), 0);

	terminal_writestring("\nIDT base: ", 0);
	terminal_writestring(to_hex64(idt_ptr.base), 0);
	terminal_writestring(" limit: ", 0);
	terminal_writestring(to_hex32(idt_ptr.limit), 0);
	terminal_writestring("\n\n", 0);

	terminal_writestring("", 1);

	for(;;)
	{
		__asm__ volatile ("hlt");
	}
}