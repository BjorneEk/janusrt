
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#include "irq.h"
#include "uart.h"

irq_fn_t spi_table[1020-32]; /* SPIs 32..1019 */
irq_fn_t ppi_table[32];      /* 0..31 */


void irq_register_ppi(u32 intid, irq_fn_t fn)  /* intid < 32 */
{
	if (intid < 32) {
		uart_puts("register ppi (");
		uart_putu64((u64)(uintptr_t)fn);
		uart_puts(")\n");
		ppi_table[intid] = fn;
	}
}

void irq_register_spi(u32 intid, irq_fn_t fn)  /* 32..1019 */
{
	if (intid >= 32 && intid < 1020) {
		uart_puts("register spi (");
		uart_putu64((u64)(uintptr_t)fn);
		uart_puts(")\n");
		spi_table[intid-32] = fn;
	}
}

void irq_default_handler(u32 intid)
{
	uart_puts("default irq\n");
	return;
}

void irq_init(void)
{
	int i;

	for (i = 0; i < 1020; ++i) {
		if (i < 32)
			irq_register_ppi(i, (irq_fn_t)0);
		else
			irq_register_spi(i, (irq_fn_t)0);
	}
}

void irq_dispatch(u32 intid)
{
	irq_fn_t f;

	//uart_puts("irq (");
	//uart_putu32(intid);
	//uart_puts(")\n");

	if (intid > 1020) {
		irq_default_handler(intid);
		return;
	}
	f = (intid < 32) ? ppi_table[intid] : spi_table[intid - 32];

	//uart_puts("irq dispatch (");
	//uart_putu64((u64)(uintptr_t)f);
	//uart_puts(")\n");

	if (f)
		f();
}

