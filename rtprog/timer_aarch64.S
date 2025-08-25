
// timer functions
// Author Gustaf Franzen <gustaffranzen@icloud.com>

.section .text


	// --- GICD/GICR bases (QEMU virt) ---
	.equ	GICR_BASE0,       0x080A0000   // first redistributor frame
	.equ	GICR_STRIDE,      0x20000      // 128 KiB per CPU
	.equ	GICR_CTLR,        0x0000
	.equ	GICR_TYPER,       0x0008       // 64-bit
	.equ	GICR_WAKER,       0x0014
	.equ	GICR_SGI_BASE,    0x10000      // +64 KiB: SGI/PPI frame
	// In SGI/PPI frame:
	.equ	GICR_IGROUPR0,    0x0080
	.equ	GICR_ISENABLER0,  0x0100
	.equ	GICR_IPRIORITYR,  0x0400
	.equ	GICD_BASE,        0x08000000
	.equ	GICD_CTLR,        0x0000
	.equ	GICR_BASE,        0x080A0000

	.global gic_enable_dist
gic_enable_dist:
	// x9 = GICD base
	mov	x9, #((GICD_BASE) & 0xFFFF)
	movk	x9, #((GICD_BASE >> 16) & 0xFFFF), lsl #16
	movk	x9, #((GICD_BASE >> 32) & 0xFFFF), lsl #32
	movk	x9, #((GICD_BASE >> 48) & 0xFFFF), lsl #48

	ldr	w10, [x9, #GICD_CTLR]
	orr	w10, w10, #(1 << 4)     // ARE_NS
	orr	w10, w10, #(1 << 1)     // EnableGrp1NS
	str	w10, [x9, #GICD_CTLR]
	ret

// ---------------- Minimal GICv3 bring-up for PPI 30 (timer) ------------------
	.global gicr_base_for_this_cpu
//gicr_base_for_this_cpu:
//	stp	x29, x30, [sp, #-16]!
//	mov	x29, sp

	// MPIDR_EL1 -> pack (Aff3:Aff2:Aff1:Aff0) into x2 (32-bit value in x2)
//	mrs	x1, MPIDR_EL1
//	and	x2, x1, #0xFF             // Aff0
//	ubfx	x3, x1, #8,  #8           // Aff1
//	ubfx	x4, x1, #16, #8           // Aff2
//	ubfx	x5, x1, #32, #8           // Aff3  <-- was 'ubfx w5,...' (bad)
//	orr	x2, x2, x3, lsl #8
//	orr	x2, x2, x4, lsl #16
//	orr	x2, x2, x5, lsl #24       // x2 holds packed affinity

	// x0 = first redistributor frame (GICR_BASE0)
//	mov	x0, #((GICR_BASE0) & 0xFFFF)
//	movk	x0, #((GICR_BASE0 >> 16) & 0xFFFF), lsl #16
//	movk	x0, #((GICR_BASE0 >> 32) & 0xFFFF), lsl #32
//	movk	x0, #((GICR_BASE0 >> 48) & 0xFFFF), lsl #48

//1:	// Read 64-bit TYPER and compare its Affinity value (bits [63:32])
//	ldr	x6, [x0, #GICR_TYPER]
//	lsr	x7, x6, #32               // x7[31:0] = redistributor affinity
//	cmp	w7, w2                    // compare low 32 bits
//	beq	2f
	// If LAST==1 (bit 4), stop; else step to next frame
//	tbnz	x6, #4, 2f
//	add	x0, x0, #GICR_STRIDE
//	b	1b

//2:	ldp	x29, x30, [sp], #16
//	ret

// ----------------------- Minimal GIC bring-up for PPI 30 --------------------
//	.global gic_enable_ppi30
//gic_enable_ppi30:
//	stp	x29, x30, [sp, #-16]!
//	mov	x29, sp

	// Enable ICC_* sysregs at EL1
//	mrs	x10, ICC_SRE_EL1
//	orr	x10, x10, #1
//	msr	ICC_SRE_EL1, x10
//	isb

	// Ensure distributor is on (your gic_enable_dist as before)
//	bl	gic_enable_dist

	// Find my redistributor base (CPU-aware)
//	bl	gicr_base_for_this_cpu
//	mov	x9, x0                      // x9 = rdist base
	// SGI/PPI frame base
//	add	x12, x9, #GICR_SGI_BASE

	// Wake redistributor: clear ProcessorSleep, wait ChildrenAsleep==0
//	ldr	w10, [x9, #GICR_WAKER]
//	bic	w10, w10, #(1 << 1)
//	str	w10, [x9, #GICR_WAKER]
//1:	ldr	w10, [x9, #GICR_WAKER]
//	tst	w10, #(1 << 2)
//	b.ne	1b

	// Put PPI 30 in Group1NS
//	ldr	w10, [x12, #GICR_IGROUPR0]
//	orr	w10, w10, #(1 << 30)
//	str	w10, [x12, #GICR_IGROUPR0]

	// Priority for INTID 30 (byte write)
//	add	x11, x12, #GICR_IPRIORITYR
//	add	x11, x11, #30
//	mov	w10, #0x80
//	strb	w10, [x11]

	// Enable PPI 30
//	mov	w10, #(1 << 30)
//	str	w10, [x12, #GICR_ISENABLER0]

	// CPU IF: unmask priorities & enable Group 1
//	mov	w10, #0xFF
//	msr	ICC_PMR_EL1, x10
//	isb
//	mov	w10, #1
//	msr	ICC_IGRPEN1_EL1, x10
//	isb

//	ldp	x29, x30, [sp], #16
//	ret

// ---------------- Periodic timer start (N ticks in x0) -----------------------
//	.global g_ticks_per_period
//	.bss
//	.align 3
//	.global g_period_ticks
//g_period_ticks:
//	.quad 0		// 64-bit period in counter ticks
//	.global g_next_cval
//g_next_cval:
//	.quad 0		// next absolute deadline (CVAL)
//g_ticks_per_period:
//	.word 0
//	.text
//.text
//	.global start_periodic_task_ticks
// x0 = period in ticks (64-bit)
//start_periodic_task_ticks:
//	stp	x29, x30, [sp, #-16]!
//	mov	x29, sp

	// Save period
//	adrp	x9, g_period_ticks
//	add	x9, x9, :lo12:g_period_ticks
//	str	x0, [x9]

	// Bring up GIC (PPI 30)
//	bl	gic_enable_ppi30

	// First deadline = now + period
//	mrs	x10, CNTPCT_EL0
//	add	x10, x10, x0
//	msr	CNTP_CVAL_EL0, x10

	// Persist next deadline
//	adrp	x9, g_next_cval
//	add	x9, x9, :lo12:g_next_cval
//	str	x10, [x9]
//
	// Enable timer: ENABLE=1, IMASK=0
//	mov	w10, #1
//	msr	CNTP_CTL_EL0, x10

	// Unmask IRQs globally
//	msr	DAIFClr, #2
//	isb

//	ldp	x29, x30, [sp], #16
//	ret
//	.global start_periodic_task_hz
// x0 = frequency in Hz (32/64-bit ok)
//start_periodic_task_hz:
//	stp	x29, x30, [sp, #-16]!
//	mov	x29, sp

//	mrs	x9, CNTFRQ_EL0		// cycles per second
	// Avoid div by zero
//	cbnz	x0, 1f
//	mov	x0, #1
//1:	udiv	x0, x9, x0		// ticks = CNTFRQ / Hz
//	bl	start_periodic_task_ticks

//	ldp	x29, x30, [sp], #16
//	ret
