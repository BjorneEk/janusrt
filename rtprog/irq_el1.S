
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
.text
	.align 4
	.global irq_el1
	.extern irq_dispatch
irq_el1_X:
	sub	sp, sp, #176

	stp	x0,  x1,  [sp, #0]
	stp	x2,  x3,  [sp, #16]
	stp	x4,  x5,  [sp, #32]
	stp	x6,  x7,  [sp, #48]
	stp	x8,  x9,  [sp, #64]
	stp	x10, x11, [sp, #80]
	stp	x12, x13, [sp, #96]
	stp	x14, x15, [sp, #112]
	stp	x16, x17, [sp, #128]
	stp	x29, x30, [sp, #144]

	/* Read IAR and stash it for EOIR later */
	mrs	x0, ICC_IAR1_EL1
	str	x0, [sp, #160]

	/* Extract INTID (bits[23:0]) and call C dispatcher: void irq_dispatch(u32) */
	and	w0, w0, #0x00FFFFFF
///	mov	w1, w0
	bl	irq_dispatch

	/* EOI with the exact IAR we read (not just INTID) */
	ldr	x0, [sp, #160]
	msr	ICC_EOIR1_EL1, x0

	/* If you ever set EOImode==1, also:
	 *   msr ICC_DIR_EL1, x0
	 */

	/* Restore regs and return to interrupted context */
	ldp	x29, x30, [sp, #144]
	ldp	x16, x17, [sp, #128]
	ldp	x14, x15, [sp, #112]
	ldp	x12, x13, [sp, #96]
	ldp	x10, x11, [sp, #80]
	ldp	x8,  x9,  [sp, #64]
	ldp	x6,  x7,  [sp, #48]
	ldp	x4,  x5,  [sp, #32]
	ldp	x2,  x3,  [sp, #16]
	ldp	x0,  x1,  [sp, #0]

	add	sp, sp, #176
	eret

.text
	.align  7
	#include "structs.h"
	.equ KERNEL_SP,		(SCHED_P0 + PROC_CTX + CTX_SP)
	.equ KERNEL_MMAP,	(SCHED_P0 + PROC_CTX + CTX_MMAP)

	.extern mmu_map_switch
	.extern dump_map

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
	//ldr	x21, [x21, :lo12:G_SCHED]

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
	add	x22, x20, #CTX_MMAP

	// call C handler
	mov	x0, x20
	bl	irq_dispatch

	//refetch current ctx
	ldr	x20, [x21, #SCHED_CURR] // x0 = G_SCHED.curr - G_SCHED
	add	x20, x20, #PROC_CTX

	//DEBUG
//	add	x0, x20, #CTX_MMAP
//	cmp	x0, x22
//	b.eq	no_dump
//	mov	x0, x22
//	bl	dump_map
//	add	x0, x20, #CTX_MMAP
//	bl	dump_map
//no_dump:

	//switch to ctx MMAP
	add	x0, x20, #CTX_MMAP
	bl	mmu_map_switch

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

//typedef JRT_PACKED struct ctx {
//	u64	x[31];
//	u64	sp;
//	u64	pc;
//	u64	pstate;
//
//	u128 vregs[32];
//	u32 fpsr, fpcr;
//	mmu_map_t mmap;
//} ctx_t;

.text
	.align  7

// ---- ctx layout offsets (match ctx_t above) ----
	.equ CTX_OFF_X,        0
	.equ CTX_OFF_SP,       (31*8)
	.equ CTX_OFF_PC,       (CTX_OFF_SP + 8)
	.equ CTX_OFF_PSTATE,   (CTX_OFF_PC + 8)
	.equ CTX_OFF_VREGS,    (CTX_OFF_PSTATE + 8)
	.equ CTX_OFF_FPSR,     (CTX_OFF_VREGS + 32*16)
	.equ CTX_OFF_FPCR,     (CTX_OFF_FPSR + 4)
	.equ CTX_OFF_MMAP,     (CTX_OFF_FPCR + 4)        // 792

	// mmu_map_t subfields (packed)
	.equ MMAP_OFF_L0,      0                         // pt_root_t.l0
	.equ MMAP_OFF_L0PA,    8                         // pt_root_t.l0_pa
	.equ MMAP_OFF_ASID,    16                        // u16 asid
	.equ MMAP_SIZE,        18

	// Pad ctx size to 16-byte alignment (AAPCS64)
	.equ CTX_SIZE,         (CTX_OFF_MMAP + 24)

	.equ SAVE_FP, 0

	.extern irq_dispatch
	.extern G_KERNEL_CTX

//	.global irq_el1
	.type   irq_el1_, %function
irq_el1_:
	// On EL1 IRQ entry, hardware already switched to SP_EL1 (kernel stack)
	// and set PSTATE.I=1 (IRQs masked). We keep IRQs masked for simplicity.

	mrs     x28, TPIDR_EL1          // x27 = &irq_sp_top
	mov     x26, sp                 // save old SP_EL1 (optional)
	mov     sp,  x28                // now on IRQ stack

	// Snapshot minimal state before we touch SP much
	mrs     x16, SP_EL0            // possible interrupted SP (if SPSR.SP==0)
	mrs     x17, ELR_EL1           // interrupted PC
	mrs     x18, SPSR_EL1          // interrupted PSTATE
	mrs     x19, TTBR0_EL1         // current map: base | (ASID<<48)

	// Allocate ctx_t frame on kernel stack
	sub     sp, sp, #CTX_SIZE
	mov     x21, sp                // x21 = ctx*

	// Save GPRs x0..x29, then x30
	stp     x0,  x1,  [x21, #(CTX_OFF_X +  0)]
	stp     x2,  x3,  [x21, #(CTX_OFF_X + 16)]
	stp     x4,  x5,  [x21, #(CTX_OFF_X + 32)]
	stp     x6,  x7,  [x21, #(CTX_OFF_X + 48)]
	stp     x8,  x9,  [x21, #(CTX_OFF_X + 64)]
	stp     x10, x11, [x21, #(CTX_OFF_X + 80)]
	stp     x12, x13, [x21, #(CTX_OFF_X + 96)]
	stp     x14, x15, [x21, #(CTX_OFF_X + 112)]
	stp     x16, x17, [x21, #(CTX_OFF_X + 128)]  // SP_EL0 (x16), ELR (x17)
	stp     x18, x19, [x21, #(CTX_OFF_X + 144)]  // SPSR (x18), TTBR0 (x19)
	stp     x20, x21, [x21, #(CTX_OFF_X + 160)]  // scratch & ctx* (for debug)
	stp     x22, x23, [x21, #(CTX_OFF_X + 176)]
	stp     x24, x25, [x21, #(CTX_OFF_X + 192)]
	stp     x26, x27, [x21, #(CTX_OFF_X + 208)]
	stp     x28, x29, [x21, #(CTX_OFF_X + 224)]
	str     x30,      [x21, #(CTX_OFF_X + 240)]

	// ctx->sp: choose SP_EL0 or SP_EL1 based on SPSR_EL1.SP (bit 0)
	// If SPSR.SP==0 ->
	// interrupted used SP_EL0; else SP_EL1 (current sp before sub).
	tbnz    x18, #0, 1f
	// SP_EL0 case
	str     x16, [x21, #CTX_OFF_SP]
	b       2f
1:  // SP_EL1 case: reconstruct entry SP_EL1 (current sp + CTX_SIZE)
	add     x0,  sp,  #CTX_SIZE
	str     x0,  [x21, #CTX_OFF_SP]
2:
	// ctx->pc / pstate
	str     x17, [x21, #CTX_OFF_PC]
	str     x18, [x21, #CTX_OFF_PSTATE]

#if SAVE_FP
	// Enable FP/ASIMD access at EL1 (if not already)
	mrs     x9, CPACR_EL1
	mov     x10, x9
	orr     x10, x10, #(3 << 20)        // FPEN=0b11
	msr     CPACR_EL1, x10
	isb

	// Save Q registers (512 bytes)
	// Note: CTX_OFF_VREGS is 16-byte aligned;
	// stack is 16-byte aligned by AAPCS64.
	stp     q0,  q1,  [x21, #(CTX_OFF_VREGS +  16*0)]
	stp     q2,  q3,  [x21, #(CTX_OFF_VREGS +  16*2)]
	stp     q4,  q5,  [x21, #(CTX_OFF_VREGS +  16*4)]
	stp     q6,  q7,  [x21, #(CTX_OFF_VREGS +  16*6)]
	stp     q8,  q9,  [x21, #(CTX_OFF_VREGS +  16*8)]
	stp     q10, q11, [x21, #(CTX_OFF_VREGS +  16*10)]
	stp     q12, q13, [x21, #(CTX_OFF_VREGS +  16*12)]
	stp     q14, q15, [x21, #(CTX_OFF_VREGS +  16*14)]
	stp     q16, q17, [x21, #(CTX_OFF_VREGS +  16*16)]
	stp     q18, q19, [x21, #(CTX_OFF_VREGS +  16*18)]
	stp     q20, q21, [x21, #(CTX_OFF_VREGS +  16*20)]
	stp     q22, q23, [x21, #(CTX_OFF_VREGS +  16*22)]
	stp     q24, q25, [x21, #(CTX_OFF_VREGS +  16*24)]
	stp     q26, q27, [x21, #(CTX_OFF_VREGS +  16*26)]
	stp     q28, q29, [x21, #(CTX_OFF_VREGS +  16*28)]
	stp     q30, q31, [x21, #(CTX_OFF_VREGS +  16*30)]
	// Save FPSR/FPCR
	mrs     x11, FPSR
	mrs     x12, FPCR
	str     w11, [x21, #CTX_OFF_FPSR]
	str     w12, [x21, #CTX_OFF_FPCR]
	// (We keep CPACR_EL1 changes;
	// if you lazily manage FP, you’d restore x9 here.)
#endif
	// ---- Save current MMU map into ctx->mmap ----
	// l0_pa := TTBR0 base; asid := TTBR0>>48; l0 (VA) := l0_pa (identity)
	and     x0,  x19, #~0xFFF              // x0 = TTBR0 base (4K aligned)
	lsr     x1,  x19, #48                  // x1 = ASID
	str     x0,  [x21, #(CTX_OFF_MMAP + MMAP_OFF_L0PA)]
	str     x0,  [x21, #(CTX_OFF_MMAP + MMAP_OFF_L0)]
	strh    w1,  [x21, #(CTX_OFF_MMAP + MMAP_OFF_ASID)]

	// ---- Read IAR now (ack the interrupt); keep full IAR for EOIR ----
	mrs     x22, ICC_IAR1_EL1       // x22 = IAR
	and     w23, w22, #0x00FFFFFF   // w23 = intid (low 24 bits)

	// ---- Switch to kernel map from G_KERNEL_CTX->mmap ----
	adrp    x2,  G_KERNEL_CTX
	add     x2,  x2, :lo12:G_KERNEL_CTX
	ldr     x3,  [x2]                       // x3 = G_KERNEL_CTX (ctx_t*)
						// load kernel mmap.l0_pa and asid
	ldr     x4,  [x3, #(CTX_OFF_MMAP + MMAP_OFF_L0PA)]
	ldrh    w5,  [x3, #(CTX_OFF_MMAP + MMAP_OFF_ASID)]
	// compose TTBR0 := (asid<<48) | base
	and     x4,  x4, #~0xFFF
	uxth    x5,  w5
	lsl     x5,  x5, #48
	orr     x4,  x4, x5
	msr     TTBR0_EL1, x4
	isb

	// ---- Call C dispatcher: irq_dispatch(ctx, intid) ----
	mov     x0,  x21                // arg0 = ctx*
	mov     w1,  w23                // arg1 = intid
	bl      irq_dispatch

	// ---- EOIR using the saved IAR ----
	msr     ICC_EOIR1_EL1, x22
	isb

	// ---- Switch to ctx->mmap (may be a different process now) ----
	ldr     x6,  [x21, #(CTX_OFF_MMAP + MMAP_OFF_L0PA)]
	ldrh    w7,  [x21, #(CTX_OFF_MMAP + MMAP_OFF_ASID)]
	and     x6,  x6, #~0xFFF
	uxth    x7,  w7
	lsl     x7,  x7, #48
	orr     x6,  x6, x7
	msr     TTBR0_EL1, x6
	isb

	// ---- Restore PC/PSTATE from ctx ----
	ldr     x8,  [x21, #CTX_OFF_PC]
	ldr     x9,  [x21, #CTX_OFF_PSTATE]
	msr     ELR_EL1,  x8
	msr     SPSR_EL1, x9

#if SAVE_FP
	// Restore FPSR/FPCR
	ldr     w11, [x21, #CTX_OFF_FPSR]
	ldr     w12, [x21, #CTX_OFF_FPCR]
	msr     FPSR, x11
	msr     FPCR, x12
	// Restore Q regs
	ldp     q0,  q1,  [x21, #(CTX_OFF_VREGS +  16*0)]
	ldp     q2,  q3,  [x21, #(CTX_OFF_VREGS +  16*2)]
	ldp     q4,  q5,  [x21, #(CTX_OFF_VREGS +  16*4)]
	ldp     q6,  q7,  [x21, #(CTX_OFF_VREGS +  16*6)]
	ldp     q8,  q9,  [x21, #(CTX_OFF_VREGS +  16*8)]
	ldp     q10, q11, [x21, #(CTX_OFF_VREGS +  16*10)]
	ldp     q12, q13, [x21, #(CTX_OFF_VREGS +  16*12)]
	ldp     q14, q15, [x21, #(CTX_OFF_VREGS +  16*14)]
	ldp     q16, q17, [x21, #(CTX_OFF_VREGS +  16*16)]
	ldp     q18, q19, [x21, #(CTX_OFF_VREGS +  16*18)]
	ldp     q20, q21, [x21, #(CTX_OFF_VREGS +  16*20)]
	ldp     q22, q23, [x21, #(CTX_OFF_VREGS +  16*22)]
	ldp     q24, q25, [x21, #(CTX_OFF_VREGS +  16*24)]
	ldp     q26, q27, [x21, #(CTX_OFF_VREGS +  16*26)]
	ldp     q28, q29, [x21, #(CTX_OFF_VREGS +  16*28)]
	ldp     q30, q31, [x21, #(CTX_OFF_VREGS +  16*30)]
#endif
	// ---- Restore GPRs ----
	ldp     x0,  x1,  [x21, #(CTX_OFF_X +  0)]
	ldp     x2,  x3,  [x21, #(CTX_OFF_X + 16)]
	ldp     x4,  x5,  [x21, #(CTX_OFF_X + 32)]
	ldp     x6,  x7,  [x21, #(CTX_OFF_X + 48)]
	ldp     x8,  x9,  [x21, #(CTX_OFF_X + 64)]
	ldp     x10, x11, [x21, #(CTX_OFF_X + 80)]
	ldp     x12, x13, [x21, #(CTX_OFF_X + 96)]
	ldp     x14, x15, [x21, #(CTX_OFF_X + 112)]
	ldp     x16, x17, [x21, #(CTX_OFF_X + 128)]
	ldp     x18, x19, [x21, #(CTX_OFF_X + 144)]
	ldp     x20, x21, [x21, #(CTX_OFF_X + 160)]
	ldp     x22, x23, [x21, #(CTX_OFF_X + 176)]
	ldp     x24, x25, [x21, #(CTX_OFF_X + 192)]
	ldp     x26, x27, [x21, #(CTX_OFF_X + 208)]
	ldp     x28, x29, [x21, #(CTX_OFF_X + 224)]
	ldr     x30,      [x21, #(CTX_OFF_X + 240)]

	// Pop ctx frame
	add     sp, sp, #CTX_SIZE
	ldr     x12, [x21, #CTX_OFF_SP]
	// x21 still holds ctx* in your code; reload if needed
	tbz     x9, #0, 3f  // if SP bit == 0 → EL1t
	msr     SP_EL1, x12 // EL1h return: set SP_EL1 now (safe: on IRQ stack)
	b       4f
3:	msr     SP_EL0, x12               // EL1t return: set SP_EL0
4:
	eret
















