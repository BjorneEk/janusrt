ENTRY(_start)

SECTIONS
{
	. = 0x90000000;

	.text : {
		KEEP(*(.text.boot))
		*(.text*)
	}

	.rodata : {
		*(.rodata*)
	}

	.data : {
		*(.data*)
	}

	.bss : {
		__bss_start = .;
		*(.bss*)
		*(COMMON)
		*(.bss.mailbox*)
		__bss_end = .;
	}

	/DISCARD/ : {
		*(.comment)
		*(.note*)
	}
}
