
// Author Gustaf Franzen <gustaffranzen@icloud.com>

#ifndef _GIC_H_
#define _GIC_H_
#include "types.h"
#include "sched.h"
/* ---- MMIO bases for QEMU virt ---- */
#define GICD_BASE		0x08000000UL
#define GICR_BASE0		0x080A0000UL	/* first redistributor frame */
#define GICR_STRIDE		0x00020000UL	/* 128 KiB per CPU */

/* ---- GICD/GICR offsets ---- */
#define GICD_CTLR         0x0000
#define GICD_IGROUPR(n)   (0x0080 + 4*(n))
#define GICD_IGRPMODR(n)  (0x0D00 + 4*(n))
#define GICD_ISENABLER(n) (0x0100 + 4*(n))
#define GICD_ICENABLER(n) (0x0180 + 4*(n))
#define GICD_ICFGR(n)     (0x0C00 + 4*(n))
#define GICD_IPRIORITYR   0x0400
#define GICD_ISPENDR(n)   (0x0200 + 4*(n))
#define GICD_ICPENDR(n)   (0x0280 + 4*(n))
#define GICD_IROUTER(i)   (0x6000 + 8*(i))

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

static inline unsigned r32i(unsigned id){ return id / 32; }
static inline unsigned b32i(unsigned id){ return id % 32; }
static inline unsigned icfgr_i(unsigned id){ return id / 16; }
static inline unsigned icfgr_sh(unsigned id){ return (id % 16) * 2; }
static inline unsigned prio_off(unsigned id){ return id; }

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
static inline u64 sys_mrs_mpidr(void)
{
	u64 v;
	asm volatile("mrs %0, MPIDR_EL1" : "=r"(v));
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

static inline u64 irouter_aff(u8 a3, u8 a2, u8 a1, u8 a0)
{
	return ((u64)a3<<48)|((u64)a2<<32)|((u64)a1<<16)|a0;
}
extern void gic_enable_dist(void);

void gic_enable_spi(u32 spi);

int gic_enable_ppi(u32 ppi, u8 prio, int group1ns);

uintptr_t gicr_base_for_this_cpu(void);
#endif

