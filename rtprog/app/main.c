#include "uart.h"
#include "syscall.h"
#include "timer.h"

int main(void *mem, u64 mem_size)
{
	unsigned long long now_us, dl_us, dl_ticks;
	uart_puts("started with ");
	uart_putu64(mem_size);
	uart_puts(" bytes att address: ");
	uart_puthex((uintptr_t)mem);
	now_us = time_now_us();
	dl_us = now_us + 5000000ULL;
	dl_ticks = ticks_from_us(dl_us);


	uart_puts("now: ");
	uart_putu64(now_us);
	uart_puts(" us\n");
	uart_puts("deadline: ");
	uart_putu64(dl_us);
	uart_puts(" us\n");
	uart_puts("deadline: ");
	uart_putu64(dl_ticks);
	uart_puts(" ticks\n");



	uart_puts("\n waiting 1 sec\n");
	syscall(SYSCALL_WAIT_UNTIL, dl_ticks);
	uart_puts("\nterminating\n");
	return 0;
}
