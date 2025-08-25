
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#include "uart.h"
void uart_putc(char c)
{
	volatile u32 *fr;
	volatile u32 *dr;

	fr = (u32 *)UART_FR;
	dr = (u32 *)UART_DR;

	while (*fr & UART_FR_TXFF)
		;

	*dr = (u32)(u8)c;
}

void uart_puts(const char *s)
{
	const char *p;

	p = s;
	while (*p)
		uart_putc(*p++);
}

void uart_putu64(uint64_t v)
{
	char buf[21];
	int i;

	i = 20;
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

void uart_putu32(uint32_t v)
{
	uart_putu64(v);
}
static char tohex(int i)
{
	if (i < 10)
		return '0' + i;
	return 'A' + (i - 10);
}
void uart_puthex(uint64_t v)
{
	char buf[17];
	int i;

	i = 16;
	buf[i--] = '\0';

	if (v == 0) {
		uart_putc('0');
		return;
	}

	while (v && i >= 0) {
		buf[i--] = tohex(v % 16);
		v /= 16;
	}
	uart_puts(&buf[i+1]);
}


