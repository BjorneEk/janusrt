ENTRY(_start)

SECTIONS {
	. = JRT_MEM_PHYS;

	.text : {
		KEEP(*(.text._start))
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
		__bss_end = .;
	}

	_end = .;
}
