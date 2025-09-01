
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
.text
	.align 4
	.global irq_el1
	.extern irq_dispatch
irq_el1:
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




