

ENTRY(main)

PHDRS {
	text   PT_LOAD FLAGS(5); /* R|X */
	rodata PT_LOAD FLAGS(4); /* R   */
	data   PT_LOAD FLAGS(6); /* R|W */
}

SECTIONS {
	. = 0x0;

	.text : {
		KEEP(*(.text.main))
		*(.text*) *(.init*) *(.fini*)
	} :text
	.rodata : ALIGN(16) { *(.rodata*) *(.constdata*) } :rodata
	.data   : ALIGN(16) { *(.data*) } :data
	.bss (NOLOAD) : ALIGN(16) {
		__bss_start = .;
		*(.bss*) *(COMMON)
		__bss_end = .;
	} :data

	/DISCARD/ : { *(.note*) *(.comment*) }
}
