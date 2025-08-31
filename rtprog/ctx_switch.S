
//typedef struct ctx {
//	u64	x[31];
//	u64	sp;
//	u64	pc;
//	u64	pstate;
//
//	u128 vregs[32];
//	u32 fpsr, fpcr;
//} ctx_t;

//ovoid store_pstate(ctx_t *ctx);
.text
	.align  2
	.global store_pstate
	.type   store_pstate, %function

// Offsets into ctx_t (see notes above)
	.equ X_OFF,        0x000
	.equ SP_OFF,       0x0F8
	.equ PC_OFF,       0x100
	.equ PSTATE_OFF,   0x108
	.equ VREGS_OFF,    0x110
	.equ FPSR_OFF,     0x310
	.equ FPCR_OFF,     0x314

store_pstate:
	// -------- Save GPRs x0..x30 to curr->x[] --------
	// Pair stores; offsets are multiples of 16 bytes (scaled by 8 for STP)
	stp     x0,  x1,  [x0, #(X_OFF +  0)]
	stp     x2,  x3,  [x0, #(X_OFF + 16)]
	stp     x4,  x5,  [x0, #(X_OFF + 32)]
	stp     x6,  x7,  [x0, #(X_OFF + 48)]
	stp     x8,  x9,  [x0, #(X_OFF + 64)]
	stp     x10, x11, [x0, #(X_OFF + 80)]
	stp     x12, x13, [x0, #(X_OFF + 96)]
	stp     x14, x15, [x0, #(X_OFF + 112)]
	stp     x16, x17, [x0, #(X_OFF + 128)]
	stp     x18, x19, [x0, #(X_OFF + 144)]
	stp     x20, x21, [x0, #(X_OFF + 160)]
	stp     x22, x23, [x0, #(X_OFF + 176)]
	stp     x24, x25, [x0, #(X_OFF + 192)]
	stp     x26, x27, [x0, #(X_OFF + 208)]
	stp     x28, x29, [x0, #(X_OFF + 224)]
	str     x30,      [x0, #(X_OFF + 240)]     // x30 (LR)

	// Save SP
	mov     x2, sp
	str     x2, [x0, #SP_OFF]

	// Save "PC": use LR (return address of caller) as resume PC
	str     x30, [x0, #PC_OFF]

	// Save PSTATE (pack NZCV|DAIF into u64: low32=NZCV, high32=DAIF)
	mrs     x2, nzcv                  // NZCV in low bits
	mrs     x3, daif                  // DAIF in low nibble
	orr     x2, x2, x3, lsl #32
	str     x2, [x0, #PSTATE_OFF]

	// -------- Save FPSIMD --------
	// Save v0..v31 (q regs), and FPSR/FPCR
	add     x2, x0, #VREGS_OFF
	stp     q0,  q1,  [x2, #(  0)]
	stp     q2,  q3,  [x2, #( 32)]
	stp     q4,  q5,  [x2, #( 64)]
	stp     q6,  q7,  [x2, #( 96)]
	stp     q8,  q9,  [x2, #(128)]
	stp     q10, q11, [x2, #(160)]
	stp     q12, q13, [x2, #(192)]
	stp     q14, q15, [x2, #(224)]
	stp     q16, q17, [x2, #(256)]
	stp     q18, q19, [x2, #(288)]
	stp     q20, q21, [x2, #(320)]
	stp     q22, q23, [x2, #(352)]
	stp     q24, q25, [x2, #(384)]
	stp     q26, q27, [x2, #(416)]
	stp     q28, q29, [x2, #(448)]
	stp     q30, q31, [x2, #(480)]

	mrs     x3, fpsr
	mrs     x4, fpcr
	str     w3, [x0, #FPSR_OFF]
	str     w4, [x0, #FPCR_OFF]

.text
	.align  2
	.global load_pstate
	.type   load_pstate, %function
// u64 load_pstate(ctx_t *c);
// returns nothing;
load_pstate:
	// -------- Restore new context from x1 --------
	// Restore FPSIMD first (keeps x1 as base intact)
	add     x2, x0, #VREGS_OFF
	ldp     q0,  q1,  [x2, #(  0)]
	ldp     q2,  q3,  [x2, #( 32)]
	ldp     q4,  q5,  [x2, #( 64)]
	ldp     q6,  q7,  [x2, #( 96)]
	ldp     q8,  q9,  [x2, #(128)]
	ldp     q10, q11, [x2, #(160)]
	ldp     q12, q13, [x2, #(192)]
	ldp     q14, q15, [x2, #(224)]
	ldp     q16, q17, [x2, #(256)]
	ldp     q18, q19, [x2, #(288)]
	ldp     q20, q21, [x2, #(320)]
	ldp     q22, q23, [x2, #(352)]
	ldp     q24, q25, [x2, #(384)]
	ldp     q26, q27, [x2, #(416)]
	ldp     q28, q29, [x2, #(448)]
	ldp     q30, q31, [x2, #(480)]

	ldr     w3, [x0, #FPSR_OFF]
	ldr     w4, [x0, #FPCR_OFF]
	msr     fpsr, x3
	msr     fpcr, x4

	// Restore GPRs except x0/x1 (keep x1 as base until the end)
	ldp     x2,  x3,  [x0, #(X_OFF + 16)]
	ldp     x4,  x5,  [x0, #(X_OFF + 32)]
	ldp     x6,  x7,  [x0, #(X_OFF + 48)]
	ldp     x8,  x9,  [x0, #(X_OFF + 64)]
	ldp     x10, x11, [x0, #(X_OFF + 80)]
	ldp     x12, x13, [x0, #(X_OFF + 96)]
	ldp     x14, x15, [x0, #(X_OFF + 112)]
	ldp     x16, x17, [x0, #(X_OFF + 128)]
	ldp     x18, x19, [x0, #(X_OFF + 144)]
	ldp     x20, x21, [x0, #(X_OFF + 160)]
	ldp     x22, x23, [x0, #(X_OFF + 176)]
	ldp     x24, x25, [x0, #(X_OFF + 192)]
	ldp     x26, x27, [x0, #(X_OFF + 208)]
	ldp     x28, x29, [x0, #(X_OFF + 224)]
	ldr     x30,      [x0, #(X_OFF + 240)]

	// ---- Prepare PC, SP, PSTATE using only x16/x17 scratch ----
	ldr     x16, [x0, #PC_OFF]        // x16 := target PC
	ldr     x17, [x0, #SP_OFF]        // x17 := target SP
	mov     sp,  x17

	ldr     x17, [x0, #PSTATE_OFF]    // low32 NZCV, high32 DAIF
	msr     nzcv, x17
	lsr     x17, x17, #32
	msr     daif, x17

	// ---- Load x1 into x17 first, then x0 (we still have ctx ptr in x0) ----
	ldr     x17, [x0, #(X_OFF + 8)]   // staged x1
	ldr     x0,  [x0, #(X_OFF + 0)]   // restore x0 (consumes ctx pointer)
	mov     x1,  x17                  // restore x1

	// ---- Jump to saved PC (x16) ----
	br      x16                        // no return
	// no return
