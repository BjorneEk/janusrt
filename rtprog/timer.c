
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#include "timer.h"
#include "uart.h"
uintptr_t gicr_base_for_this_cpu(void)
{
	u64		mpidr;
	u32		aff0, aff1, aff2, aff3;
	u32		want;
	uintptr_t	base;
	u64		typer;
	u32		have;

	asm volatile("mrs %0, MPIDR_EL1" : "=r"(mpidr));

	aff0 = (u32)(mpidr & 0xFFu);
	aff1 = (u32)((mpidr >> 8) & 0xFFu);
	aff2 = (u32)((mpidr >> 16) & 0xFFu);
	aff3 = (u32)((mpidr >> 32) & 0xFFu);
	want = aff0 | (aff1 << 8) | (aff2 << 16) | (aff3 << 24);

	base = (uintptr_t)GICR_BASE0;
	for (;;) {
		typer = mmio_r64(base + GICR_TYPER);
		have  = (u32)(typer >> 32);
		if (have == want)
			return base;

		/* if LAST==1 and no match, give up */
		if (typer & (1ull << 4))
			break;

		base += GICR_STRIDE;
	}
	/* fail hard for now: return first frame (will obviously not work) */
	return (uintptr_t)GICR_BASE0;
}
int gic_enable_ppi(u32 ppi, u8 prio, int group1ns)
{
	uintptr_t	rd_base;
	uintptr_t	sgi_base;
	u32		val;
	uintptr_t	addr;

	if (ppi < 16u || ppi > 31u)
		return -1;

	sys_enable_icc_el1();
	gic_enable_dist();

	rd_base = gicr_base_for_this_cpu();
	sgi_base = rd_base + GICR_SGI_BASE;

	/* Wake redistributor */
	val = mmio_r32(rd_base + GICR_WAKER);
	val &= ~GICR_WAKER_ProcessorSleep;
	mmio_w32(rd_base + GICR_WAKER, val);
	for (;;) {
		val = mmio_r32(rd_base + GICR_WAKER);
		if ((val & GICR_WAKER_ChildrenAsleep) == 0)
			break;
	}

	/* Group selection */
	val = mmio_r32(sgi_base + GICR_IGROUPR0);
	if (group1ns)
		val |= (1u << ppi);
	else
		val &= ~(1u << ppi);
	mmio_w32(sgi_base + GICR_IGROUPR0, val);

	/* Priority byte */
	addr = sgi_base + GICR_IPRIORITYR + ppi;
	*(volatile u8 *)addr = prio;

	/* Enable bit */
	val = (1u << ppi);
	mmio_w32(sgi_base + GICR_ISENABLER0, val);

	return 0;
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

void timer_irq_handler(void)
{
	u64	now;
	u64	next;
	u64	period;


	now = 0;
	next = g_next_cval;

	g_timer_func();

	period = g_period_ticks;

	/* skip forward until next > now */
	asm volatile("mrs %0, CNTPCT_EL0" : "=r"(now));
	while (next <= now)
		next = next + period;

	g_next_cval = next;
	asm volatile("msr CNTP_CVAL_EL0, %0" :: "r"(next));
}
