	.section .text._start
	.global _start
	.extern jrt_main
	.extern psci_cpu_off
	.extern periodic_func
	.extern start_periodic_task_hz

_start:
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

	// Your hardware-defined stack address (keep!)
	ldr	x0, =0x50100000
	mov	sp, x0

	// Install vector table
	ldr	x0, =vector_table
	msr	VBAR_EL1, x0
	isb

	// Start periodic timer: N ticks
	mov	x0, #1
	bl	start_periodic_task_hz

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
	.global timer_irq_handler
	.extern periodic_func
timer_irq_handler:
	stp	x29, x30, [sp, #-16]!
	mov	x29, sp

	bl	periodic_func
	// next_cval += period; program it
	adrp	x9, g_next_cval
	add	x9, x9, :lo12:g_next_cval
	ldr	x10, [x9]			// next_cval
	adrp	x11, g_period_ticks
	add	x11, x11, :lo12:g_period_ticks
	ldr	x12, [x11]			// period
	add	x10, x10, x12			// advance
	str	x10, [x9]
	msr	CNTP_CVAL_EL0, x10

	ldp	x29, x30, [sp], #16
	ret

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
	sub	sp, sp, #16
	mrs	x0, ICC_IAR1_EL1
	str	x0, [sp]
	and	w1, w0, #0xFFFFFF      // INTID
	mov	w2, #30                 // CNTPNSIRQ
	cmp	w1, w2
	b.ne	1f
	bl	timer_irq_handler
	b	2f
1:
	ldr	x0, [sp]
	add	sp, sp, #16
	msr	ICC_EOIR1_EL1, x0
	b	bad_irq_spin
2:
	ldr	x0, [sp]
	add	sp, sp, #16
	msr	ICC_EOIR1_EL1, x0
	eret

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
