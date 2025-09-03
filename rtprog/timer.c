
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#include "timer.h"
static u64	g_next_cval;      /* absolute next deadline (CNTPCT units) */
static u64	g_period_ticks;   /* period in ticks */

static timer_func_t g_timer_func;

void set_timer_func(timer_func_t tf)
{
	g_timer_func = tf;
}

void start_periodic_task(u64 ticks)
{
	u64	now;
	int	rc;
	u32	ctl;

	g_period_ticks = ticks;

	/* enable EL1 physical timer PPI = 30, prio mid (0x80), Group1NS */
	rc = gic_enable_ppi(30u, 0x80u, 1);
	(void)rc;

	now = sys_mrs_cntpct();
	g_next_cval = now + ticks;
	sys_msr_cntp_cval(g_next_cval);

	ctl = 1u;	/* ENABLE=1, IMASK=0 */
	sys_msr_cntp_ctl(ctl);

	/* unmask IRQs in PSTATE */
	asm volatile("msr DAIFClr, #2\n\tisb");
}

void start_periodic_task_freq(u64 hz)
{
	u64	frq;
	u64	ticks;

	frq = sys_mrs_cntfrq();
	if (hz == 0)
		hz = 1;
	ticks = frq / hz;

	start_periodic_task(ticks);
}

void stop_periodic_task(void)
{
	u32	v;

	v = 0;
	asm volatile("msr CNTP_CTL_EL0, %0" :: "r"(v));
	asm volatile("isb");
}

void timer_irq_handler(ctx_t *c)
{
	u64	now;
	u64	next;
	u64	period;


	now = 0;
	next = g_next_cval;

	g_timer_func(c);

	period = g_period_ticks;

	/* skip forward until next > now */
	asm volatile("mrs %0, CNTPCT_EL0" : "=r"(now));
	while (next <= now)
		next = next + period;

	g_next_cval = next;
	asm volatile("msr CNTP_CVAL_EL0, %0" :: "r"(next));
}
