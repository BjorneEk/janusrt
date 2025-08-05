; Author: Gustaf Franzen <gustaffranzen@icloud.com>

	.global psci_cpu_on
	.type psci_cpu_on, %function

psci_cpu_on:
	mov	x0, #0x84000003		// PSCI_CPU_ON
	mov	x3, x2			// context
	mov	x2, x1			// entry
	mov	x1, x0			// core_id
	mov	x0, #0x84000003
	smc	#0
	ret
