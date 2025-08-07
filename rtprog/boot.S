

.section .text._start
.global _start
//.section .text
//.global _start
.extern jrt_main
.extern psci_cpu_off

_start:
	// Set up stack pointer (example stack at 0x50100000)
	ldr	x0, =0x50100000
	mov	sp, x0

	// Set up dummy exception vector table
	ldr	x0, =vector_table
	msr	VBAR_EL1, x0
	isb

	// Jump to jrt_main
	bl	jrt_main

	//turn of cpu
	mov	x0, #0x3
	mov	x1, #0x0
	mov	x2, #0x0
	bl	psci_cpu_off

	// If jrt_main returns, spin
hang:
	wfe
	b hang

.align 11
vector_table:
	.space 0x400
