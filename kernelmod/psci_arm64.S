// Author: Gustaf Franzen <gustaffranzen@icloud.com>

	.global psci_cpu_on_arm64
	.type psci_cpu_on_arm64, %function

// x0: core_id
// x1: entry
// x2: context
psci_cpu_on_arm64:
	mov	x3, x2			// context
	mov	x2, x1			// entry
	mov	x1, x0			// core_id
	// load 0x84000003, (PSCI_CPU_OFF)
	// load 0xc4000003, (PSCI_CPU_ON)
	movz	x0, #0x0003
	movk	x0, #0xc400, lsl #16
//	smc	#0
	hvc	#0
	ret

	.section .modinfo, "a"
	.asciz "license=GPL"
	.asciz "author=Gustaf Franzen"
	.asciz "description=ARM PSCI CPU boot helper"
