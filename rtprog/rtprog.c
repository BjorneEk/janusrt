/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */

#include <stdint.h>
#include "mailbox.h"
#include "rtcore.h"

#define UART_BASE	(0x09000000)
//#define UART_BASE	(0x09000000)
#define UART_TXFIFO	(UART_BASE + 0x00)
#define UART_FR		(UART_BASE + 0x18)

#define UART_FR_TXFF	(1 << 5)

static void uart_putc(char c)
{
	volatile uint32_t *fr;
	volatile uint32_t *tx;

	fr = (uint32_t *)UART_FR;
	tx = (uint32_t *)UART_TXFIFO;

	while (*fr & UART_FR_TXFF)
		;

	*tx = c;
}

static void uart_puts(char *s)
{
	while (*s)
		uart_putc(*s++);
}

static void uart_putu32(uint32_t v)
{
	char buf[11];
	int i;

	i = 10;
	buf[i--] = '\0';

	if (v == 0) {
		uart_putc('0');
		return;
	}

	while (v && i >= 0) {
		buf[i--] = '0' + (v % 10);
		v /= 10;
	}
	uart_puts(&buf[i+1]);
}

void jrt_main(void)
{
	uint32_t i;
	uart_puts("a number: ");
	uart_putu32(69);
	uart_puts("\n");

	for (i = 0; i < 100; ++i) {
		uart_puts("[jrt] Hello from JRT! ");
		uart_putu32(i);
		uart_puts("\n");
	}
}
/*
void memcpy(u8 *to, u8 *from, size_t sz)
{
	size_t i;

	for (i = 0; i < sz; ++i)
		to[i] = from[i];
}

static inline int mpsc_pop(struct mpsc_ring *r, u8 out[16])
{
	u32 tail;
	u32 idx;

	tail = r->tail;
	idx = tail & TOJRT_MASK;

	if (!__atomic_load_n(&r->flags[idx], __ATOMIC_ACQUIRE))
		return -11;

	memcpy(out, (u8*)&r->data[idx], 16);

	__atomic_store_n(&r->flags[idx], 0u, __ATOMIC_RELEASE);
	r->tail = tail + 1;
	return 0;
}
*/
//static rtcore_mem_t *rtcore_mem = (void*)JRT_MEM_PHYS;
void periodic_func(void)
{
	//u8 data[16];
	//int budget;
	uart_puts("periodic call\n");
	//for (budget = 0; budget < 64; budget++) {
	//	if (mpsc_pop(&rtcore_mem->ring, data) != 0)
	//		break;
	//	uart_puts((char*)data);
	//}
}

/*
__attribute__((section(".bss.mailbox")))
struct mailbox mbox;
void _start(void)
{
	uint32_t i, j;
	mbox.counter = 0;
	mbox.last_value = 42;

	j = 0;
	for (;;) {
		mbox.counter++;
		mbox.last_value += 3;
		uart_putu32(j);
		for (i = 0; i < 1000; ++i)
			;
		j += i;
		uart_puts("RT: ");
		uart_putu32(j);
		uart_putc('\n');
	}
}
*/
