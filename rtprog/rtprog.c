/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */

#include <stdint.h>
#include "mailbox.h"

#define UART_BASE	(0x09010000)
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
	uart_puts("a number: ");
	uart_putu32(69);
	uart_puts("\n");

	for (volatile int i = 0; i < 100000; ++i)
		uart_puts("[jrt] Hello from JRT!\n");
	//__asm__ volatile (
	//	"brk #0xdead\n"
	//);
	while (1)
		;
		//__asm__ volatile ("wfi"); // or just empty loop
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
