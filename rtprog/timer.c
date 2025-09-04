
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#include "timer.h"


static u64 g_cntfrq;


void timebase_init(void)
{
	g_cntfrq = sys_mrs_cntfrq();
}

u64 time_now_ticks(void)
{
	return sys_mrs_cntpct();
}

u64 ticks_from_ns(u64 ns)
{
	u64 sec, rem, t, num;

	sec = ns / 1000000000ULL;
	rem = ns % 1000000000ULL;

	t = sec * g_cntfrq;

	// rem*g_cntfrq fits in 64-bit (<= ~1e18 for typical frq), then divide by 1e9
	num = rem * g_cntfrq + 500000000ULL;   // +0.5e9 for rounding
	t += num / 1000000000ULL;

	return t;
}

u64 ticks_from_us(u64 us)
{
	u64 sec, rem, t, num;

	sec = us / 1000000ULL;
	rem = us % 1000000ULL;

	t = sec * g_cntfrq;

	num = rem * g_cntfrq + 500000ULL;      // +0.5e6 for rounding
	t += num / 1000000ULL;

	return t;
}

u64 ns_from_ticks(u64 t)
{
	u64 sec, rem, ns, num;

	sec = t / g_cntfrq;
	rem = t % g_cntfrq;

	ns = sec * 1000000000ULL;

	// rem*1e9 can be up to ~1e9*1e9 = 1e18 (fits in 64-bit)
	num = rem * 1000000000ULL;
	ns += num / g_cntfrq;

	return ns;
}

u64 us_from_ticks(u64 t)
{
	u64 sec, rem, us, num;

	sec = t / g_cntfrq;
	rem = t % g_cntfrq;

	us = sec * 1000000ULL;
	num = rem * 1000000ULL;
	us += num / g_cntfrq;

	return us;
}

u64 time_now_ns(void)
{
	return ns_from_ticks(time_now_ticks());
}

u64 time_now_us(void)
{
	return us_from_ticks(time_now_ticks());
}

u64 timer_schedule_at_ticks(u64 deadline_ticks)
{
	u64 now;

	now = time_now_ticks();
	if ((s64)(deadline_ticks - now) <= 0) {
		deadline_ticks = now + 1;       // nudge into the future by 1 tick
	}

	// Program compare first, then enable+unmask
	sys_msr_cntp_cval(deadline_ticks);
	// CNTP_CTL: bit0 ENABLE, bit1 IMASK (0=unmask), bit2 ISTATUS (RO)
	sys_msr_cntp_ctl( (1u<<0) /*ENABLE*/ | (0u<<1) /*IMASK=0*/ );

	asm volatile("isb");
	return deadline_ticks;
}

void timer_cancel(void)
{
	sys_msr_cntp_ctl(0);    // disable
	asm volatile("isb");
}

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
