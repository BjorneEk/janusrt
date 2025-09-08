/*#include "uart.h"
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
*/
#include "uart.h"
#include "syscall.h"
#include "timer.h"

void wait(u32 sec, void *mem, u64 mem_size, u32 p);
void func(void *mem, u64 mem_size, void *arg);
int main(void *mem, u64 mem_size)
{

	unsigned long long now_us, dl_us, dl_ticks;
	now_us = time_now_us();
	dl_us = now_us + 5000000ULL;
	dl_ticks = ticks_from_us(dl_us);

	syscall(SYSCALL_SPAWN, dl_ticks, (uintptr_t)func, NULL, 0xFF);
	for (int i = 0; i < 10; ++i)
		wait(2, mem, mem_size, 1);
	uart_puts("main exit\n");
	return 0;
}
void wait(u32 sec, void *mem, u64 mem_size, u32 p)
{
	unsigned long long now_us, dl_us, dl_ticks;
	uart_puts("proc: ");
	uart_putu32(p);
	uart_puts("\n");
	uart_puts("started with ");
	uart_putu64(mem_size);
	uart_puts(" bytes att address: ");
	uart_puthex((uintptr_t)mem);
	now_us = time_now_us();
	dl_us = now_us + (sec * 1000000ULL);
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



	uart_puts("\n waiting ");
	uart_putu32(sec);
	uart_puts(" sec\n");
	syscall(SYSCALL_WAIT_UNTIL, dl_ticks);
	uart_puts("\nterminating\n");
	return;
}
void func(void *mem, u64 mem_size, void *arg)
{
	for (u32 i = 0; i < 10; ++i)
		wait(1, mem, mem_size, 2);
	uart_puts("func exit\n");
}
//static void spawn(u64 deadline, u64 ep, u64 ap, u64 mem_req)


