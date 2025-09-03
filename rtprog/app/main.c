#include "uart.h"

int main(void *mem, u64 mem_size)
{
	uart_puts("started with ");
	uart_putu64(mem_size);
	uart_puts(" bytes att address: ");
	uart_puthex((uintptr_t)mem);
	uart_puts("\nterminating\n");
	return 0;
}
