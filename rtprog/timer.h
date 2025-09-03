
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _TIMER_H_
#define _TIMER_H_
#include "gic.h"


void timebase_init(void);

u64 ticks_from_ns(u64 ns);
u64 ticks_from_us(u64 us);
u64 ns_from_ticks(u64 t);
u64 us_from_ticks(u64 t);
u64 time_now_ns(void);
u64 time_now_us(void);
u64 time_now_ticks(void);

u64 timer_schedule_at_ticks(u64 deadline_ticks);
typedef void (*timer_func_t)(ctx_t *c);
#define EL1_PHYS_TIMER_PPI (30)
#define EL1_VIRT_TIMER_PPI (27)



void set_timer_func(timer_func_t tf);
void start_periodic_task(u64 ticks);
void start_periodic_task_freq(u64 hz);
void stop_periodic_task(void);

void timer_irq_handler(ctx_t *c);
#endif
