global long_mode_start
global framebuffer_mapped_addr

extern kernel_main
extern framebuffer_addr
extern page_table_l2

extern framebuffer_pitch
extern framebuffer_height

section .text
bits 64
long_mode_start:
    ; load null into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; -------------------------
    ; Map framebuffer at VA 0x01000000
    ; -------------------------
    mov rax, [framebuffer_addr]

    ; align physical framebuffer address down to 2MiB
    mov rbx, rax
    and rbx, -0x200000

    mov esi, [framebuffer_pitch]
    mov ecx, [framebuffer_height]
    mov edx, esi
    imul edx, ecx

    mov ecx, edx
    add ecx, 0x1FFFFF
    shr ecx, 21

    mov qword [framebuffer_mapped_addr], 0x01000000
    mov rdi, [framebuffer_mapped_addr]
    mov r8, rdi
    shr r8, 21
    and r8, 0x1FF

.map_loop:
    mov rdx, rbx
    or rdx, 0x83
    mov [page_table_l2 + r8*8], rdx

    add rbx, 0x200000
    inc r8
    dec ecx
    jnz .map_loop

    call kernel_main
    hlt

section .bss
align 8
framebuffer_mapped_addr: dq 0