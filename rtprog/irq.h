
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _IRQ_H_
#define _IRQ_H_

#include "types.h"
#include "sched.h"
typedef void (*irq_fn_t)(ctx_t*);

void irq_init(void);
void irq_register_ppi(u32 intid, irq_fn_t fn);
void irq_register_spi(u32 intid, irq_fn_t fn);
#endif
