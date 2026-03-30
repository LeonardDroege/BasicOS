#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "memory.h"
#include "keyboard.h"
#include "pic.h"
#include "idt.h"
#include "gdt.h"

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000 

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
	
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_scroll_up(void) {
    // Move rows 1 through VGA_HEIGHT-1 up by one row
    // Copy from row 1 to row 0, row 2 to row 1, etc.
    for (size_t row = 1; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            size_t src_index = row * VGA_WIDTH + col;
            size_t dest_index = (row - 1) * VGA_WIDTH + col;
            terminal_buffer[dest_index] = terminal_buffer[src_index];
        }
    }
    
    // Clear the last row
    for (size_t col = 0; col < VGA_WIDTH; col++) {
        size_t last_row_index = (VGA_HEIGHT - 1) * VGA_WIDTH + col;
        terminal_buffer[last_row_index] = vga_entry(' ', terminal_color);
    }
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c, char should_print_start_char) 
{
	if(c == '\n')
	{
		terminal_row++;
        terminal_column = 0;

        if (terminal_row == VGA_HEIGHT)
		{   
            terminal_scroll_up();
            terminal_row = VGA_HEIGHT - 1;
        }
		if(should_print_start_char == 1) terminal_writestring("> ", 0);
		return;
	}

	if(c == '\r')
	{
		terminal_column = 0;
		if(should_print_start_char == 1) terminal_writestring("> ", 0);
		return;
	}

	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
		{
            terminal_scroll_up();
            terminal_row = VGA_HEIGHT - 1;
        }
	}
}

void terminal_write(const char* data, size_t size, char should_print_start_char) 
{
	for (size_t i = 0; i < size; i++)
	{
        terminal_putchar(data[i], should_print_start_char);
    }
}

void terminal_writestring(const char* data, char should_print_start_char) 
{
	terminal_write(data, strlen(data), should_print_start_char);
}

void terminal_backspace()
{
	if(terminal_column > 0)
	{
		terminal_column--;
		size_t index = terminal_row * VGA_WIDTH + terminal_column;
		terminal_buffer[index] = vga_entry(' ', terminal_color);
	}
}

void print_memory_map()
{
	for(size_t i = 0; i < usable_count; i++)
	{
		terminal_writestring("[Usable] ", 0);
		terminal_writestring(to_hex32((uint32_t)usable_memory[i].base), 0);
		terminal_writestring(" - ", 0);
		terminal_writestring(to_hex32((uint32_t)(usable_memory[i].length + usable_memory[i].base)), 0);
		terminal_writestring(" ", 0);

		uint32_t size = usable_memory[i].length / 1024;
		char isMb = size > 1024 ? 1 : 0;
		size = isMb == 1 ? size / 1024 : size;
		terminal_writestring("(", 0);
		terminal_writestring(to_dec(size), 0);
		terminal_writestring(isMb == 1 ?"MB)\n" : "KB)\n", 0);
	}
}

void kheap_dump(void) {
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

    terminal_writestring("========================\n", 0);
}

void isr0_handler()
{
    terminal_writestring("Divide by zero!\n", 1);
    for(;;);
}

void kernel_main(uint32_t magic, multiboot_info_t* mb_info) 
{
	terminal_initialize();
	terminal_writestring("BasicOS\n\n", 0);

	if(magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		terminal_writestring("Bad multiboot magic!\n\n", 0);
		return;
	}
	terminal_writestring("Initialized multiboot!\n\n", 0);

	parse_memory_map(mb_info);
	print_memory_map();

	pmm_init();
	kheap_init();

	terminal_writestring("Initialized pmm and heap!\n\n", 0);

	gdt_init();
	idt_init();
	idt_install_exceptions();
	pic_remap();

	uint8_t mask = inb(0x21);
	mask |= (1 << 0);
	mask &= ~(1 << 1);
	outb(0x21, mask);

	keyboard_init();

	__asm__ volatile ("sti");

	terminal_writestring("GDT base: ", 0);
	terminal_writestring(to_hex32(gdt_ptr.base), 0);
	terminal_writestring(" limit: ", 0);
	terminal_writestring(to_hex32(gdt_ptr.limit), 0);

	terminal_writestring("\nIDT base: ", 0);
	terminal_writestring(to_hex32(idt_ptr.base), 0);
	terminal_writestring(" limit: ", 0);
	terminal_writestring(to_hex32(idt_ptr.limit), 0);
	terminal_writestring("\n\n", 0);

	terminal_writestring("> ", 0);

	for(;;)
	{
		__asm__ volatile ("hlt");
	}
}