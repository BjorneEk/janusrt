
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _TIMER_H_
#define _TIMER_H_
#include "gic.h"

typedef void (*timer_func_t)(ctx_t *c);
#define EL1_PHYS_TIMER_PPI (30)
#define EL1_VIRT_TIMER_PPI (27)



void set_timer_func(timer_func_t tf);
void start_periodic_task(u64 ticks);
void start_periodic_task_freq(u64 hz);
void stop_periodic_task(void);

void timer_irq_handler(ctx_t *c);
#endif
