// Author: Gustaf Franzen <gustaffranzen@icloud.com>

	#include "autogen/memory_layout.h"
	.equ JRT_STACK_START_CONST, JRT_STACK_START

	.section .text._start
	.global _start
	.extern jrt_main
	.extern psci_cpu_off
	.extern irq_dispatch
	.extern irq_el1

_start:
	// switch to EL1
	mrs	x0, CurrentEL
	lsr	x0, x0, #2              // 1=EL1, 2=EL2, 3=EL3
	cmp	x0, #2
	b.ne	1f

	// ---------- We are in EL2 ----------
	// Allow EL1 access to physical counter/timer
	mrs	x1, CNTHCTL_EL2
	// Bits: EL1PCTEN (bit 0), EL1PCEN (bit 1)
	orr	x1, x1, #(1 << 0)
	orr	x1, x1, #(1 << 1)
	msr	CNTHCTL_EL2, x1

	// Make EL1's virtual offset zero so CNTVCT==CNTPCT
	msr	CNTVOFF_EL2, xzr

	// Enable GICv3 sysregs at EL2 (required for EL1 sysregs to be accessible)
	mrs	x1, ICC_SRE_EL2
	orr	x1, x1, #1          // SRE=1
	msr	ICC_SRE_EL2, x1
	isb

	// Ensure we run EL1h, interrupts masked on entry; we'll unmask later
	mov	x1, #(0b0101 << 6)  // SPSR: M[3:0]=0101 (EL1h), mask FIQ+IRQ
	orr	x1, x1, #(1 << 7)   // F
	orr	x1, x1, #(1 << 6)   // I
	orr	x1, x1, #(1 << 9)   // A (optional)
	msr	SPSR_EL2, x1

	// Where to land in EL1:
	adr	x1, el1_entry
	msr	ELR_EL2, x1

	// Non-secure, 64-bit at EL1
	mrs	x1, HCR_EL2
	orr	x1, x1, #(1 << 31)   // RW=1 (EL1 is AArch64)
	msr	HCR_EL2, x1
	isb
	eret

1:
	// ---------- We are already EL1 ----------
	b	el1_entry

el1_entry:
	// Use SP_EL1 (so IRQ EL1h has a valid stack)
	msr	SPSel, #1

	// hardware-defined stack address
	ldr	x0, =JRT_STACK_START_CONST
	mov	sp, x0

	// Install vector table
	adrp	x0, vector_table
	add	x0, x0, :lo12:vector_table
	msr	VBAR_EL1, x0
	isb

	// zero out bss
	adrp    x0, __bss_start
	add     x0, x0, :lo12:__bss_start
	adrp    x1, __bss_end
	add     x1, x1, :lo12:__bss_end
1:	cmp     x0, x1
	b.hs    2f
	str     xzr, [x0], #8
	b       1b
2:
	//enable FP/ASIMD
	mrs     x0, cpacr_el1
	orr     x0, x0, #(3 << 20)     // FPEN=0b11
	msr     cpacr_el1, x0
	isb

	msr     ICC_IGRPEN0_EL1, xzr   // disable Group-0 (FIQ) delivery
	isb

	// Jump to main
	bl	jrt_main

	// Turn off CPU if we return
	mov	x0, #0x3
	mov	x1, #0x0
	mov	x2, #0x0
	bl	psci_cpu_off

hang:
	wfe
	b	hang
// ---------------- Spins for unconfigured exceptions --------------------------
fiq_spin:
	b	fiq_spin
serr_spin:
	b	serr_spin
bad_irq_spin:
	wfe
	b	bad_irq_spin
//new interrupt layout
// 1. store mmap
// 2. switch to kernel mmap
// 1. store ctx_t on (kernel) stack,
// defer, passing (ctx_t *ctx, u64 *eret_addr)
// then, handler can perform context switch by simply editing these
// restore ctx,
// switch back mmap
// eret
//

// ----------- Syncronous exception ---------------------------------------------
.text
	.align 4
	.global sync_el1h
	.extern sync_exception_entry

sync_el1h:
	// preserve LR just in case we want to return (we don't)
	sub	sp, sp, #16
	str	x30, [sp]

	mrs	x0, ESR_EL1
	mrs	x1, ELR_EL1
	mrs	x2, FAR_EL1
	bl	sync_exception_entry

	ldr	x30, [sp]
	add	sp, sp, #16
	eret

// ---------------- Vector table (128-byte entries, 2 KiB aligned) -------------
	.align 11
vector_table:
	b	sync_el1h          // EL1t Sync
	.balign 128
	b	irq_el1     // EL1t IRQ
	.balign 128
	b	fiq_spin           // EL1t FIQ
	.balign 128
	b	serr_spin          // EL1t SError
	.balign 128

	b	sync_el1h          // EL1h Sync
	.balign 128
	b	irq_el1     // EL1h IRQ
	.balign 128
	b	fiq_spin           // EL1h FIQ
	.balign 128
	b	serr_spin          // EL1h SError
	.balign 128

	b	sync_el1h          // EL0 AArch64 Sync
	.balign 128
	b	irq_el1     // EL0 AArch64 IRQ
	.balign 128
	b	fiq_spin
	.balign 128
	b	serr_spin
	.balign 128

	b	sync_el1h          // EL0 AArch32 Sync
	.balign 128
	b	irq_el1     // EL0 AArch32 IRQ
	.balign 128
	b	fiq_spin
	.balign 128
	b	serr_spin
