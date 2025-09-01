
ENTRY(_start)

PHDRS {
	text   PT_LOAD FLAGS(5); /* R|X */
	rodata PT_LOAD FLAGS(4); /* R   */
	data   PT_LOAD FLAGS(6); /* R|W */
}

SECTIONS {
	. = JRT_CODE_PHYS;
	__kernel_start = .;
	.text : {
		KEEP(*(.text._start))
		*(.text*) *(.init*) *(.fini*)
	} :text
	.rodata : ALIGN(16) { *(.rodata*) *(.constdata*) } :rodata
	.data   : ALIGN(16) { *(.data*) } :data
	.bss (NOLOAD) : ALIGN(16) {
		__bss_start = .;
		*(.bss*) *(COMMON)
		__bss_end = .;
	} :data
	__kernel_end = .;
	/DISCARD/ : { *(.note*) *(.comment*) }
}
