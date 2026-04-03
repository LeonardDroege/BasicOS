ENTRY(start)

SECTIONS
{
	. = 2M;

	kernel_start = .;

	.boot :
	{
		KEEP(*(.multiboot))
	}

	.text :
	{
		*(.text)
	}

	.rodata :
	{
		*(.rodata)
	}

	.data :
	{
		*(.data)
	}

	.bss :
	{
		*(COMMON)
		*(.bss)
	}

	kernel_end = .;

	kernel_heap_start = .;
	. = . + 4M;   /* 4 MB heap */
	kernel_heap_end = .;
}