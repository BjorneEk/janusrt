

// Author Gustaf Franzen <gustaffranzen@icloud.com>

#ifndef _TIMER_H_
#define _TIMER_H_
#include "types.h"
#include "sched.h"
/* ---- MMIO bases for QEMU virt ---- */
#define GICD_BASE		0x08000000UL
#define GICR_BASE0		0x080A0000UL	/* first redistributor frame */
#define GICR_STRIDE		0x00020000UL	/* 128 KiB per CPU */

/* ---- GICD/GICR offsets ---- */
#define GICD_CTLR		0x0000

#define GICR_CTLR		0x0000
#define GICR_TYPER		0x0008		/* 64-bit */
#define GICR_WAKER		0x0014
#define  GICR_WAKER_ProcessorSleep	(1u << 1)
#define  GICR_WAKER_ChildrenAsleep	(1u << 2)

#define GICR_SGI_BASE		0x10000		/* SGI/PPI frame base (+64KiB) */
#define GICR_IGROUPR0		0x0080
#define GICR_ISENABLER0		0x0100
#define GICR_IPRIORITYR		0x0400		/* byte-addressable priorities */

#define GICR_IGRPMODR0  0x0D00  /* if RAZ/WI, writes are harmless */

static inline u32 mmio_r32(uintptr_t a)
{
	u32	v;

	v = *(volatile u32 *)a;
	return v;
}

static inline void mmio_w32(uintptr_t a, u32 v)
{
	*(volatile u32 *)a = v;
}

static inline u64 mmio_r64(uintptr_t a)
{
	u64	v;

	v = *(volatile u64 *)a;
	return v;
}

/* sysregs */
static inline u64 sys_mrs_cntfrq(void)
{
	u64	v;

	asm volatile("mrs %0, CNTFRQ_EL0" : "=r"(v));
	return v;
}

static inline u64 sys_mrs_cntpct(void)
{
	u64	v;

	asm volatile("mrs %0, CNTPCT_EL0" : "=r"(v));
	return v;
}

static inline void sys_msr_cntp_cval(u64 v)
{
	asm volatile("msr CNTP_CVAL_EL0, %0" :: "r"(v));
}

static inline void sys_msr_cntp_ctl(u32 v)
{
	asm volatile("msr CNTP_CTL_EL0, %0" :: "r"(v));
}

static inline void sys_enable_icc_el1(void)
{
	u64	v;

	/* SRE=1 */
	asm volatile("mrs %0, ICC_SRE_EL1" : "=r"(v));
	v |= 1u;
	asm volatile("msr ICC_SRE_EL1, %0" :: "r"(v));
	asm volatile("isb");

	/* unmask priorities */
	v = 0xFF;
	asm volatile("msr ICC_PMR_EL1, %0" :: "r"(v));
	asm volatile("isb");

	/* enable Group 1 */
	v = 1;
	asm volatile("msr ICC_IGRPEN1_EL1, %0" :: "r"(v));
	asm volatile("isb");
}

extern void gic_enable_dist(void);

typedef void (*timer_func_t)(ctx_t *c);
#define EL1_PHYS_TIMER_PPI (30)
#define EL1_VIRT_TIMER_PPI (27)



uintptr_t gicr_base_for_this_cpu(void);
int gic_enable_ppi(u32 ppi, u8 prio, int group1ns);
void set_timer_func(timer_func_t tf);
void start_periodic_task(u64 ticks);
void start_periodic_task_freq(u64 hz);
void stop_periodic_task(void);

void timer_irq_handler(ctx_t *c);
#endif
