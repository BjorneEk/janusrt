
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _UART_H_
#define _UART_H_

#include "types.h"
#define UART_BASE       0x09000000UL

#define UART_DR         (UART_BASE + 0x000)  /* Data register (TX/RX) */
#define UART_FR         (UART_BASE + 0x018)  /* Flag register */
#define UART_IBRD       (UART_BASE + 0x024)
#define UART_FBRD       (UART_BASE + 0x028)
#define UART_LCRH       (UART_BASE + 0x02C)
#define UART_CR         (UART_BASE + 0x030)
#define UART_IMSC       (UART_BASE + 0x038)
#define UART_ICR        (UART_BASE + 0x044)

/* FR bits */
#define UART_FR_TXFF    (1u << 5)  /* TX FIFO full */
#define UART_FR_RXFE    (1u << 4)  /* RX FIFO empty */

/* Minimal safe init (8N1, FIFO on, TX/RX enable) */
static inline void uart_init(void)
{
	volatile u32 *cr;
	volatile u32 *lcr;

	cr  = (u32 *)UART_CR;
	lcr = (u32 *)UART_LCRH;

	/* Disable while we tweak */
	*cr = 0;

	/* 8N1, FIFO enable: WLEN=11 (bits 6-5), FEN=1 (bit 4) */
	*lcr = (3u << 5) | (1u << 4);

	/* Enable TX, RX, UART */
	*cr = (1u << 8) | (1u << 9) | (1u << 0);
}
void uart_putc(char c);

void uart_puts(const char *s);

void uart_putu32(uint32_t v);
void uart_putu64(uint64_t v);

void uart_puthex(uint64_t v);
#endif
