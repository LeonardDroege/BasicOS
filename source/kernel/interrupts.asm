.intel_syntax noprefix

.global gdt_flush
.global idt_load
.global irq1_stub
.global isr0_stub

.extern gdt_ptr
.extern idt_ptr
.extern keyboard_handler
.extern isr0_handler

gdt_flush:
    lgdt gdt_ptr
    mov %ax, 0x10
    mov %ds, %ax
    mov %es, %ax
    mov %fs, %ax
    mov %gs, %ax
    mov %ss, %ax
    jmp 0x08:.flush
.flush:
    ret

idt_load:
    lidt idt_ptr
    ret

irq1_stub:
    pusha
    call keyboard_handler
    popa
    iret

isr0_stub:
    pusha
    call isr0_handler
    popa
    iret