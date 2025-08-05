/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */

#include <stdint.h>
#include "mailbox.h"

#define UART_BASE	(0x09000000UL)
#define UART_PTR(_off)	((volatile uint32_t *)(UART_BASE + (_off)))
#define UART_DR		UART_PTR(0x00)
#define UART_FR		UART_PTR(0x18)
#define FR_TXFF		(1 << 5)

__attribute__((section(".bss.mailbox")))
struct mailbox mbox;

static void uart_putc(char c)
{
	while ((*UART_FR) & FR_TXFF)
		;
	*UART_DR = c;
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
