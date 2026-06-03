section .multiboot
header_start:
	; magic number
	dd 0xe85250d6 ; multiboot2
	; architecture
	dd 0 ; protected mode i386
	; header length
	dd header_end - header_start
	; checksum
	dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

	; start framebuffer tag
	dw 5
	dw 0
	dd 20
	dd 2560
	dd 1440
	dd 32
	dd 0

	; end tag
	dw 0
	dw 0
	dd 8
header_end: