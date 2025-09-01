// AArch64 EL1 MMU helpers (GAS)

	.text
	.align  3

	.global el1_mmu_on
	.type   el1_mmu_on, %function
// void el1_mmu_on(uint64_t mair, uint64_t tcr, uint64_t ttbr0_pa)
el1_mmu_on:
	msr     MAIR_EL1, x0
	isb
	msr     TCR_EL1,  x1
	isb
	msr     TTBR0_EL1, x2
	isb
	dsb     sy
	tlbi    vmalle1
	dsb     sy
	isb
	mrs     x3, SCTLR_EL1
	orr     x3, x3, #(1 << 0)      // M
	orr     x3, x3, #(1 << 2)      // C
	orr     x3, x3, #(1 << 12)     // I
			// WXN left clear: you requested RWX everywhere
	msr     SCTLR_EL1, x3
	isb
	ret

	.global write_ttbr0_asid
	.type   write_ttbr0_asid, %function
// void write_ttbr0_asid(uint64_t ttbr0_pa, uint16_t asid)
write_ttbr0_asid:
	and     x0, x0, #~(0xFFF)      // 4 KiB alignment
	uxth    x1, w1
	lsl     x1, x1, #48            // ASID in [63:48] (A1=0)
	orr     x0, x0, x1
	msr     TTBR0_EL1, x0
	isb
	ret

	.global tlbi_all
	.type   tlbi_all, %function
tlbi_all:
	dsb     sy
	tlbi    vmalle1
	dsb     sy
	isb
	ret

	.global tlbi_asid
	.type   tlbi_asid, %function
// void tlbi_asid(uint16_t asid)
tlbi_asid:
	uxth    x0, w0
	lsl     x0, x0, #48
	dsb     ish
	tlbi    aside1is, x0
	dsb     ish
	isb
	ret

