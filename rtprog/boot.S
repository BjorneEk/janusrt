	.section .text._start
	.global _start
	.extern jrt_main
	.extern psci_cpu_off
	.extern irq_dispatch
	.extern start_periodic_task_hz

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
	ldr	x0, =0x51000000
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

// ---------------- Timer IRQ helper (calls periodic_func + re-arm) ------------
//	.global timer_irq_handler
//	.extern periodic_func
//timer_irq_handler:
//	stp	x29, x30, [sp, #-16]!
//	mov	x29, sp
//
//	bl	periodic_func
//	// next_cval += period; program it
//	adrp	x9, g_next_cval
//	add	x9, x9, :lo12:g_next_cval
//	ldr	x10, [x9]			// next_cval
//	adrp	x11, g_period_ticks
//	add	x11, x11, :lo12:g_period_ticks
//	ldr	x12, [x11]			// period
//	add	x10, x10, x12			// advance
//	str	x10, [x9]
//	msr	CNTP_CVAL_EL0, x10

//	ldp	x29, x30, [sp], #16
//	ret

// ---------------- Spins for unconfigured exceptions --------------------------
sync_spin:
	b	sync_spin
fiq_spin:
	b	fiq_spin
serr_spin:
	b	serr_spin
bad_irq_spin:
	wfe
	b	bad_irq_spin

// ---------------- IRQ entry (EL1) --------------------------------------------
	.align 3
irq_common_el1:
	/* Save volatile GPRs x0..x17 and FP/LR; plus a slot for saved IAR.
	 * Frame size = 176 bytes (multiple of 16 for AAPCS64).
	 *
	 * Layout:
	 *   [sp+000] x0   [sp+008] x1
	 *   [sp+016] x2   [sp+024] x3
	 *   [sp+032] x4   [sp+040] x5
	 *   [sp+048] x6   [sp+056] x7
	 *   [sp+064] x8   [sp+072] x9
	 *   [sp+080] x10  [sp+088] x11
	 *   [sp+096] x12  [sp+104] x13
	 *   [sp+112] x14  [sp+120] x15
	 *   [sp+128] x16  [sp+136] x17
	 *   [sp+144] x29  [sp+152] x30
	 *   [sp+160] saved IAR (x0 value read from ICC_IAR1_EL1)
	 */
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
//	sub	sp, sp, #16
//	mrs	x0, ICC_IAR1_EL1
//	str	x0, [sp]
//	and	w1, w0, #0xFFFFFF        // INTID
//	mov	w2, #1020
//	cmp	w1, w2
//	b.hs	1f                       // spurious
//	mov	w0, w1                   // x0=intid
//	bl	irq_dispatch             // C: void irq_dispatch(u32)
//1:	ldr	x0, [sp]
//	add	sp, sp, #16
//	msr	ICC_EOIR1_EL1, x0
//	eret

//	sub	sp, sp, #16
//	mrs	x0, ICC_IAR1_EL1
//	str	x0, [sp]
//	and	w1, w0, #0xFFFFFF      // INTID
//	mov	w2, #30                 // CNTPNSIRQ
//	cmp	w1, w2
//	b.ne	1f
//	bl	timer_irq_handler
//	b	2f
//1:
//	ldr	x0, [sp]
//	add	sp, sp, #16
//	msr	ICC_EOIR1_EL1, x0
//	b	bad_irq_spin
//2:
//	ldr	x0, [sp]
//	add	sp, sp, #16
//	msr	ICC_EOIR1_EL1, x0
//	eret

// ---------------- Vector table (128-byte entries, 2 KiB aligned) -------------
	.align 11
vector_table:
	b	sync_spin          // EL1t Sync
	.balign 128
	b	irq_common_el1     // EL1t IRQ
	.balign 128
	b	fiq_spin           // EL1t FIQ
	.balign 128
	b	serr_spin          // EL1t SError
	.balign 128

	b	sync_spin          // EL1h Sync
	.balign 128
	b	irq_common_el1     // EL1h IRQ
	.balign 128
	b	fiq_spin           // EL1h FIQ
	.balign 128
	b	serr_spin          // EL1h SError
	.balign 128

	b	sync_spin          // EL0 AArch64 Sync
	.balign 128
	b	irq_common_el1     // EL0 AArch64 IRQ
	.balign 128
	b	fiq_spin
	.balign 128
	b	serr_spin
	.balign 128

	b	sync_spin          // EL0 AArch32 Sync
	.balign 128
	b	irq_common_el1     // EL0 AArch32 IRQ
	.balign 128
	b	fiq_spin
	.balign 128
	b	serr_spin
