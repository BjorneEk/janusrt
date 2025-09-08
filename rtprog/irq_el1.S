
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
.text
	.align  7
	#include "autogen/structs.h"
	.equ KERNEL_SP,		(SCHED_P0 + PROC_CTX + CTX_SP)
	.equ KERNEL_MMAP,	(SCHED_P0 + PROC_CTX + CTX_MMAP)
	.equ IRQ_DEBUG, 0

	.extern irq_dispatch
	.extern mmu_map_switch
	.extern dump_map
	.global irq_el1
	.type   irq_el1, %function
irq_el1:
	//get some registers to work with
	sub	sp, sp, #32
	stp	x20, x21, [sp, #0]
	stp	x0, x1,   [sp, #16]

	// this is a bit cumberome to do this way, but is neccessary
	// to make shure we dont clobber any registers here.

	// get global scheduler instance
	adrp	x21, G_SCHED
	add	x21, x21, :lo12:G_SCHED

	//get ctx
	ldr	x20, [x21, #SCHED_CURR]
	add	x20, x20, #PROC_CTX

	//store GPRs [X0:X30]
	ldp	x0, x1, [sp, #0] // original x20, x21
	stp	x0, x1, [x20, #(CTX_X + 160)]
	add	sp, sp, #16
	ldp	x0, x1, [sp, #0] // original x0, x1
	stp	x0, x1, [x20, #CTX_X]
	add	sp, sp, #16

	stp	x2,  x3,  [x20, #(CTX_X + 16)]
	stp	x4,  x5,  [x20, #(CTX_X + 32)]
	stp	x6,  x7,  [x20, #(CTX_X + 48)]
	stp	x8,  x9,  [x20, #(CTX_X + 64)]
	stp	x10, x11, [x20, #(CTX_X + 80)]
	stp	x12, x13, [x20, #(CTX_X + 96)]
	stp	x14, x15, [x20, #(CTX_X + 112)]
	stp	x16, x17, [x20, #(CTX_X + 128)]
	stp	x18, x19, [x20, #(CTX_X + 144)]
	stp	x22, x23, [x20, #(CTX_X + 176)]
	stp	x24, x25, [x20, #(CTX_X + 192)]
	stp	x26, x27, [x20, #(CTX_X + 208)]
	stp	x28, x29, [x20, #(CTX_X + 224)]
	str	x30,      [x20, #(CTX_X + 240)]

	// store PC and PSTATE
	mrs	x0, ELR_EL1	//PC
	mrs	x1, SPSR_EL1	//PSTATE
	str	x0, [x20, #CTX_PC]
	str	x1, [x20, #CTX_PSTATE]

	// Save SP
	mov	x0, sp
	str	x0, [x20, #CTX_SP]

	// save VREGS, PRSR and FPCR
#if SAVE_FP
	// Enable FP/ASIMD access at EL1 (if not already)
	mrs     x10, CPACR_EL1
	orr     x10, x10, #(3 << 20)        // FPEN=0b11
	msr     CPACR_EL1, x10
	isb

	// Save Q registers (512 bytes)
	// Note: CTX_OFF_VREGS is 16-byte aligned;
	// stack is 16-byte aligned by AAPCS64.
	stp	q0,  q1,  [x20, #(CTX_VREGS +  16*0)]
	stp	q2,  q3,  [x20, #(CTX_VREGS +  16*2)]
	stp	q4,  q5,  [x20, #(CTX_VREGS +  16*4)]
	stp	q6,  q7,  [x20, #(CTX_VREGS +  16*6)]
	stp	q8,  q9,  [x20, #(CTX_VREGS +  16*8)]
	stp	q10, q11, [x20, #(CTX_VREGS +  16*10)]
	stp	q12, q13, [x20, #(CTX_VREGS +  16*12)]
	stp	q14, q15, [x20, #(CTX_VREGS +  16*14)]
	stp	q16, q17, [x20, #(CTX_VREGS +  16*16)]
	stp	q18, q19, [x20, #(CTX_VREGS +  16*18)]
	stp	q20, q21, [x20, #(CTX_VREGS +  16*20)]
	stp	q22, q23, [x20, #(CTX_VREGS +  16*22)]
	stp	q24, q25, [x20, #(CTX_VREGS +  16*24)]
	stp	q26, q27, [x20, #(CTX_VREGS +  16*26)]
	stp	q28, q29, [x20, #(CTX_VREGS +  16*28)]
	stp	q30, q31, [x20, #(CTX_VREGS +  16*30)]
	// Save FPSR/FPCR
	mrs     x11, FPSR
	mrs     x12, FPCR
	str     w11, [x20, #CTX_FPSR]
	str     w12, [x20, #CTX_FPCR]
	// (We keep CPACR_EL1 changes;
	// if you lazily manage FP, you’d restore x9 here.)
#endif
	// current MMAP should already be kept up to date

	// switch to kernel SP
	ldr	x2, [x21, #KERNEL_SP]
	mov	sp, x2

	// switch to  kernel MMAP
	add	x0, x21, #KERNEL_MMAP
	bl	mmu_map_switch

	// read IAR and extract INTID (bits[23:0])
	mrs	x0, ICC_IAR1_EL1
	str	x0, [sp, #-16]! // now kernel stack
	and	w1, w0, #0x00FFFFFF

	// DEBUG
#if IRQ_DEBUG
	add	x22, x20, #CTX_MMAP
#endif
	// call C handler
	mov	x0, x20
	bl	irq_dispatch

	//refetch current ctx
	ldr	x20, [x21, #SCHED_CURR] // x0 = G_SCHED.curr - G_SCHED
	add	x20, x20, #PROC_CTX

	//DEBUG
#if IRQ_DEBUG
	add	x0, x20, #CTX_MMAP
	cmp	x0, x22
	b.eq	no_dump
	mov	x0, x22
	bl	dump_map
	add	x0, x20, #CTX_MMAP
	bl	dump_map
no_dump:
#endif
	//switch to ctx MMAP
	add	x0, x20, #CTX_MMAP
	bl	mmu_map_switch
	// refetch again, since we are on a new map
	//adrp	x21, G_SCHED
	//add	x21, x21, :lo12:G_SCHED
	//refetch current ctx
	//ldr	x20, [x21, #SCHED_CURR] // x0 = G_SCHED.curr - G_SCHED
	//add	x20, x20, #PROC_CTX



	// restore VREGS, PRSR and FPCR
#if SAVE_FP
	// Restore FPSR/FPCR
	ldr	w11, [x20, #CTX_FPSR]
	ldr	w12, [x20, #CTX_FPCR]
	msr	FPSR, x11
	msr	FPCR, x12
	// Restore Q regs
	ldp	q0,  q1,  [x20, #(CTX_VREGS +  16*0)]
	ldp	q2,  q3,  [x20, #(CTX_VREGS +  16*2)]
	ldp	q4,  q5,  [x20, #(CTX_VREGS +  16*4)]
	ldp	q6,  q7,  [x20, #(CTX_VREGS +  16*6)]
	ldp	q8,  q9,  [x20, #(CTX_VREGS +  16*8)]
	ldp	q10, q11, [x20, #(CTX_VREGS +  16*10)]
	ldp	q12, q13, [x20, #(CTX_VREGS +  16*12)]
	ldp	q14, q15, [x20, #(CTX_VREGS +  16*14)]
	ldp	q16, q17, [x20, #(CTX_VREGS +  16*16)]
	ldp	q18, q19, [x20, #(CTX_VREGS +  16*18)]
	ldp	q20, q21, [x20, #(CTX_VREGS +  16*20)]
	ldp	q22, q23, [x20, #(CTX_VREGS +  16*22)]
	ldp	q24, q25, [x20, #(CTX_VREGS +  16*24)]
	ldp	q26, q27, [x20, #(CTX_VREGS +  16*26)]
	ldp	q28, q29, [x20, #(CTX_VREGS +  16*28)]
	ldp	q30, q31, [x20, #(CTX_VREGS +  16*30)]
#endif

	// Restore PC/PSTATE
	ldr	x8,  [x20, #CTX_PC]
	ldr	x9,  [x20, #CTX_PSTATE]
	msr	ELR_EL1,  x8
	msr	SPSR_EL1, x9

	// pop EIO with stashed IAR
	ldr	x0, [sp], #16
	msr	ICC_EOIR1_EL1, x0
	isb

	// Set return SP *after* all our stack use (choose SP_EL0/EL1) ----
	ldr	x12, [x20, #CTX_SP]
	mov	sp, x12

	// Restore GPRs [x2:x30]
	mov	x0, x20
	ldp	x2,  x3,  [x0, #(CTX_X + 16)]
	ldp	x4,  x5,  [x0, #(CTX_X + 32)]
	ldp	x6,  x7,  [x0, #(CTX_X + 48)]
	ldp	x8,  x9,  [x0, #(CTX_X + 64)]
	ldp	x10, x11, [x0, #(CTX_X + 80)]
	ldp	x12, x13, [x0, #(CTX_X + 96)]
	ldp	x14, x15, [x0, #(CTX_X + 112)]
	ldp	x16, x17, [x0, #(CTX_X + 128)]
	ldp	x18, x19, [x0, #(CTX_X + 144)]
	ldp	x20, x21, [x0, #(CTX_X + 160)]
	ldp	x22, x23, [x0, #(CTX_X + 176)]
	ldp	x24, x25, [x0, #(CTX_X + 192)]
	ldp	x26, x27, [x0, #(CTX_X + 208)]
	ldp	x28, x29, [x0, #(CTX_X + 224)]
	ldr	x30,      [x0, #(CTX_X + 240)]

	// Restore last GPRs [x0:x1]
	ldp	x0, x1, [x0, #CTX_X]

	// return from interrupt
	eret




