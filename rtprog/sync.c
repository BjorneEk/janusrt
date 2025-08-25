
// Author: Gustaf Franzen <gustaffranzen@icloud.com>

#include "uart.h"

void sync_handler(u64 esr, u64 far)
{
	uart_puts("[SYNC] ESR="); uart_puthex(esr);
	uart_puts(" FAR="); uart_puthex(far);
	uart_puts("\n");

	while (1)
		asm volatile("wfe" ::: "memory");
}
