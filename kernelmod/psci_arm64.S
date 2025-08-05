// Author: Gustaf Franzen <gustaffranzen@icloud.com>

	.global psci_cpu_on_arm64
	.type psci_cpu_on_arm64, %function

psci_cpu_on_arm64:
	ldr	x0, =0x84000003		// PSCI_CPU_ON
	mov	x3, x2			// context
	mov	x2, x1			// entry
	mov	x1, x0			// core_id
	ldr	x0, =0x84000003
	smc	#0
	ret

	.section .modinfo, "a"
	.asciz "license=GPL"
	.asciz "author=Gustaf Franzen"
	.asciz "description=ARM PSCI CPU boot helper"
