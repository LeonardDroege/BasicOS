#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

#define INPUT_BUFFER_SIZE 128
char input_buffer[INPUT_BUFFER_SIZE];
int input_length = 0;

bool shift_pressed = false;
bool ctrl_pressed  = false;
bool alt_pressed   = false;

extern void irq1_stub();
void terminal_backspace();
void terminal_putchar(char c, char should_print_start_char);
void terminal_writestring(const char* data, char should_print_start_char);

static inline uint8_t inb(uint16_t p) {
    uint8_t r;
    __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p));
    return r;
}

static inline void outb(uint16_t p, uint8_t v);

static const char scancode_to_ascii[128] = {
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 
    '\b',    // 0x0E Backspace
    '\t',    // 0x0F Tab
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 
    '\n',    // 0x1C Enter
    0,       // 0x1D Ctrl
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,       // 0x2A Left Shift
    '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,       // 0x36 Right Shift
    '*',
    0,       // 0x38 Alt
    ' ',     // 0x39 Space
    0,       // CapsLock
    0,0,0,0,0,0,0,0,0,0, // F1–F10
    0,       // NumLock
    0,       // ScrollLock
    0,       // Home
    0,       // Up
    0,       // PageUp
    '-',
    0,       // Left
    0,
    0,       // Right
    '+',
    0,       // End
    0,       // Down
    0,       // PageDown
    0,       // Insert
    0,       // Delete
    0,0,0,0,0,0,0, // F11–F12
};

static const char scancode_to_ascii_shift[128] = {
    0,   27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\b',    // Backspace
    '\t',    // Tab
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n',    // Enter
    0,       // Ctrl
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,       // Left Shift
    '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,       // Right Shift
    '*',
    0,       // Alt
    ' ',     // Space
    0,       // CapsLock
    0,0,0,0,0,0,0,0,0,0, // F1–F10
    0,       // NumLock
    0,       // ScrollLock
    0,       // Home
    0,       // Up
    0,       // PageUp
    '-',
    0,       // Left
    0,
    0,       // Right
    '+',
    0,       // End
    0,       // Down
    0,       // PageDown
    0,       // Insert
    0,       // Delete
    0,0,0,0,0,0,0, // F11–F12
};

char to_lower(char c)
{
    if(c >= 'A' && c <= 'Z')
    {
        return c + 32;
    }
    return c;
}

char to_upper(char c)
{
    if(c >= 'a' && c <= 'z')
    {
        return c - 32;
    }
    return c;
}

void str_tolower(char* s)
{
    for(size_t i = 0; s[i]; i++)
    {
        if(s[i] >= 'A' && s[i] <= 'Z')
        {
            s[i] += 32;
        }
    }
}

void str_toupper(char* s)
{
    for(size_t i = 0; s[i]; i++)
    {
        if(s[i] >= 'a' && s[i] <= 'z')
        {
            s[i] -= 32;
        }
    }
}

static void ps2_wait_input(void)
{
    while(inb(0x64) & 0x02)
    {

    }
}

static void ps2_wait_output(void)
{
    while(!(inb(0x64) & 0x01))
    {

    }
}

bool str_equals(const char* v1, const char* v2)
{
    while (*v1 && *v2) {
        if (*v1 != *v2)
            return false;
        v1++;
        v2++;
    }

    if(*v1 || *v2)
    {
        return false;
    }

    return *v1 == *v2;
}

void handle_command(char buffer[])
{
    size_t index = 0;
    char command[64];
    while(buffer[index] != ' ' && buffer[index] != '\0')
    {
        command[index] = to_lower(buffer[index]);
        index++;
    }

    if(command[index - 1] != '\0')
    {
        command[index] = '\0';
    }

    if(str_equals(command, "help"))
    {
        terminal_writestring("\nAvailable commands:\n", 1);
        terminal_writestring("==============================\n", 1);
        terminal_writestring("help\n", 1);
        terminal_writestring("==============================\n\n", 1);
        return;
    }

    terminal_writestring("\nUnknown command: \"", 1);
    terminal_writestring(command, 1);
    terminal_writestring("\"\n", 1);
    terminal_writestring("use \"help\" to list available commands\n\n", 1);
}

void keyboard_handler() {
    uint8_t sc = inb(0x60);

    if (sc & 0x80) {
        uint8_t make = sc & 0x7F;

        if(make == 0x2A || make == 0x36) shift_pressed = false;
        if(make == 0x1D) ctrl_pressed = false;
        if(make == 0x38) alt_pressed = false;

        outb(0x20, 0x20);
        return;
    }

    switch(sc)
    {
        case 0x2A:
        case 0x36:
            shift_pressed = true;
            outb(0x20, 0x20);
            return;
        case 0x1D:
            ctrl_pressed = true;
            outb(0x20, 0x20);
            return;
        case 0x38:
            alt_pressed = true;
            outb(0x20, 0x20);
            return;
        case 0x0E:
            if(input_length > 0)
            {
                input_length--;
                terminal_backspace();
            }
            outb(0x20, 0x20);
            return;
        case 0x1C:
            terminal_putchar('\n', 1);
            input_buffer[input_length] = '\0';

            handle_command(input_buffer);

            input_length = 0;
            outb(0x20, 0x20);
            return;
    }

    char c = shift_pressed ? scancode_to_ascii_shift[sc] : scancode_to_ascii[sc];

    if (c && input_length < INPUT_BUFFER_SIZE - 1)
    {
        input_buffer[input_length++] = c;
        terminal_putchar(c, 1);
    }

    outb(0x20, 0x20);
}

void idt_set_gate(int, uint64_t);

void keyboard_init()
{
    idt_set_gate(33, (uint64_t)irq1_stub);
}

#endif