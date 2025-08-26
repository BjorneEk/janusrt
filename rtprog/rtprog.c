/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */

// core includes
#include <stdint.h>

// shared includes
#include "mailbox.h"
#include "rtcore.h"

// libc includes
#include "string.h"

// jrt includes
#include "timer.h"
#include "irq.h"
#include "uart.h"

void periodic_func(void);

static void timer_trampoline(void)
{
	timer_irq_handler();
}

void timer_init(void)
{
	irq_register_ppi(EL1_PHYS_TIMER_PPI, timer_trampoline);
	set_timer_func(periodic_func);
	start_periodic_task_freq(1);
}

void jrt_main(void)
{
	uint32_t i;

	uart_init();

	irq_init();
	timer_init();
	//u8 *n = 0;
	//*n = 1;

	uart_puts("a number: ");
	uart_putu32(69);
	uart_puts("\n");

	for (i = 0; i < 100; ++i) {
		uart_puts("[jrt] Hello from JRT! ");
		uart_putu32(i);
		uart_puts("\n");
	}
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

static rtcore_mem_t *rtcore_mem = (void*)JRT_MEM_PHYS;
void periodic_func(void)
{
	u8 data[16];
	int budget;
	uart_puts("periodic call\n");
	for (budget = 0; budget < 64; budget++) {
		if (mpsc_pop(&rtcore_mem->ring, data) != 0)
			break;
		uart_puts((char*)data);
		uart_puts("\n");
	}
	//uart_puts("periodic call\n");
}

