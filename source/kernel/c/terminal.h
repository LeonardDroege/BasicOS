#ifndef TERMINAL_H
#define TERMINAL_H

#include "font.h"

extern uint64_t framebuffer_mapped_addr;
extern uint32_t framebuffer_width;
extern uint32_t framebuffer_height;
extern uint32_t framebuffer_pitch;
extern uint8_t framebuffer_bpp;

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

#define ROOT_DIRECTORY "root"
#define USER_DIRECTORY "user"

char* current_directory = ROOT_DIRECTORY;

void terminal_writestring(const char* data, char should_print_start_char);
void* kmalloc(uint32_t size);

static inline void framebuffer_putpixel(uint32_t x, uint32_t y, uint32_t color_rgb)
{
	uint32_t* framebuffer = (uint32_t*)framebuffer_mapped_addr;
	uint32_t* pixel = (uint32_t*)((uint8_t*)framebuffer + y * framebuffer_pitch + x * 4);
	*pixel = color_rgb;
}

void framebuffer_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg)
{
	for(int row = 0; row < 16; row++)
	{
		uint8_t bits = font8x16[(uint8_t)c * 16 + row];

		for(int col = 0; col < 8; col++)
		{
			uint32_t color = (bits & (1 << (7 - col))) ? fg : bg;
			framebuffer_putpixel(x + col,y + row, color);
		}
	}
}

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

#define TERMINAL_LINE_COUNT 6500 

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;

size_t terminal_width;
size_t terminal_height;

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);

uint32_t vga_to_rgb(enum vga_color c) {
    // crude mapping, tweak as you like
    switch (c) {
        case VGA_COLOR_BLACK:        return 0x000000;
        case VGA_COLOR_BLUE:         return 0x0000AA;
        case VGA_COLOR_GREEN:        return 0x00AA00;
        case VGA_COLOR_CYAN:         return 0x00AAAA;
        case VGA_COLOR_RED:          return 0xAA0000;
        case VGA_COLOR_MAGENTA:      return 0xAA00AA;
        case VGA_COLOR_BROWN:        return 0xAA5500;
        case VGA_COLOR_LIGHT_GREY:   return 0xAAAAAA;
        case VGA_COLOR_DARK_GREY:    return 0x555555;
        case VGA_COLOR_LIGHT_BLUE:   return 0x5555FF;
        case VGA_COLOR_LIGHT_GREEN:  return 0x55FF55;
        case VGA_COLOR_LIGHT_CYAN:   return 0x55FFFF;
        case VGA_COLOR_LIGHT_RED:    return 0xFF5555;
        case VGA_COLOR_LIGHT_MAGENTA:return 0xFF55FF;
        case VGA_COLOR_LIGHT_BROWN:  return 0xFFFF55;
        case VGA_COLOR_WHITE:        return 0xFFFFFF;
    }
    return 0xFFFFFF;
}

void terminal_initialize(void) 
{
	terminal_width = framebuffer_width / 8;
	terminal_height = framebuffer_height / 16;
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);

	uint32_t bg = vga_to_rgb((enum vga_color)((terminal_color >> 4) & 0x0F));
	uint32_t* framebuffer = (uint32_t*)framebuffer_mapped_addr;
	
	for (size_t y = 0; y < framebuffer_height; y++) {
		for (size_t x = 0; x < framebuffer_width; x++) {
			framebuffer[y * (framebuffer_pitch / 4) + x] = bg;
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void* memmove(void* dst, const void* src, size_t n)
{
	uint8_t* d = (uint8_t*)dst;
	const uint8_t* s = (const uint8_t*)src;

	if(d == s || n == 0)
	{
		return dst;
	}

	if(d < s)
	{
		for(size_t i = 0; i < n; i++)
		{
			d[i] = s[i];
		}
	}
	else
	{
		for(size_t i = n; i > 0; i++)
		{
			d[i-1] = s[i-1];
		}
	}

	return dst;
}

void terminal_scroll_up(void) {
	uint8_t* framebuffer = (uint8_t*)framebuffer_mapped_addr;
	size_t row_bytes = framebuffer_pitch * 16;
	size_t total_bytes = framebuffer_pitch * framebuffer_height;

    memmove(framebuffer, framebuffer + row_bytes, total_bytes - row_bytes);
    
    // Clear the last row (16px)
    uint32_t bg = vga_to_rgb((enum vga_color)((terminal_color >> 4) & 0x0F));
	uint32_t* framebuffer32 = (uint32_t*)framebuffer;
	size_t pixels_per_row = framebuffer_pitch / 4;

	for(size_t y = framebuffer_height - 16; y < framebuffer_height; y++)
	{
		for(size_t x = 0; x < framebuffer_width; x++)
		{
			framebuffer32[y * pixels_per_row + x] = bg;
		}
	}
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	if(x >= terminal_width || y >= terminal_height)
		return;

	uint32_t fg = vga_to_rgb((enum vga_color)(color & 0x0F));
	uint32_t bg = vga_to_rgb((enum vga_color)((color >> 4) & 0x0F));
	
	framebuffer_draw_char(x * 8, y * 16, c, fg, bg);
}

void terminal_putchar(char c, char should_print_start_char) 
{
	if(c == '\n')
	{
		terminal_row++;
        terminal_column = 0;

        if (terminal_row == terminal_height)
		{   
            terminal_scroll_up();
            terminal_row = terminal_height - 1;
        }
		return;
	}

	if(c == '\r')
	{
		terminal_column = 0;
		return;
	}

	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == terminal_width) {
		terminal_column = 0;
		if (++terminal_row == terminal_height)
		{
            terminal_scroll_up();
            terminal_row = terminal_height - 1;
        }
	}
}

void terminal_write(const char* data, size_t size, char should_print_start_char)
{
    if(should_print_start_char != 0)
    {
        terminal_writestring(current_directory, 0);
        terminal_writestring("> ", 0);
    }
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
	if(terminal_column == 0)
	{
		return;
	}

	terminal_column--;
	terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
}

#endif