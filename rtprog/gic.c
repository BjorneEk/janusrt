
// Author Gustaf Franzen <gustaffranzen@icloud.com>
#include "gic.h"

#include "uart.h"
uintptr_t gicr_base_for_this_cpu(void)
{
	u64		mpidr;
	u32		aff0, aff1, aff2, aff3;
	u32		want;
	uintptr_t	base;
	u64		typer;
	u32		have;


	mpidr = sys_mrs_mpidr();

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
	/* Group selection: make it Group-1 */
	val = mmio_r32(sgi_base + GICR_IGROUPR0);
	if (group1ns) val |=  (1u << ppi);     // Group-1
	else          val &= ~(1u << ppi);     // Group-0
	mmio_w32(sgi_base + GICR_IGROUPR0, val);

	/* And ensure it's **Non-secure** Group-1 (NS), not Secure Group-1 */
	if (group1ns) {
		val = mmio_r32(sgi_base + GICR_IGRPMODR0);
		val &= ~(1u << ppi);               // 0 → Group-1NS
		mmio_w32(sgi_base + GICR_IGRPMODR0, val);
	}

	/* Priority byte */
	addr = sgi_base + GICR_IPRIORITYR + ppi;
	*(volatile u8 *)addr = prio;

	/* Enable bit */
	val = (1u << ppi);
	mmio_w32(sgi_base + GICR_ISENABLER0, val);

	return 0;
}

static inline u64 build_irouter_from_mpidr(u64 mpidr, int irm_any)
{
	u8 aff0, aff1, aff2, aff3;
	u64 route;

	aff0 = (u8)( mpidr        & 0xFF);
	aff1 = (u8)((mpidr >>  8) & 0xFF);
	aff2 = (u8)((mpidr >> 16) & 0xFF);
	aff3 = (u8)((mpidr >> 32) & 0xFF);
	route = ((u64)aff3 << 56) |
		((u64)aff2 << 32) |
		((u64)aff1 << 16) |
		((u64)aff0 <<  0);
	if (irm_any)
		route |= (1u << 31);   // IRM=1: deliver to any CPU in the group
	return route;
}

// Configure SPI 'spi' to target **this CPU** (whatever core runs this code)
static void route_spi_to_this_cpu(unsigned spi)
{
	u64 mpidr;
	u64 route;

	mpidr = sys_mrs_mpidr();
	route = build_irouter_from_mpidr(mpidr, /*irm_any=*/0);

	// Program IROUTER (64-bit, done as two 32-bit writes)
	mmio_w32(GICD_BASE + GICD_IROUTER(spi) + 0, (u32)(route      ));
	mmio_w32(GICD_BASE + GICD_IROUTER(spi) + 4, (u32)(route >> 32));
}
void gic_enable_spi(u32 spi)
{
	u32 v;

	// Group-1 (non-secure) so it arrives as IRQ (not FIQ)
	v = mmio_r32(GICD_BASE + GICD_IGROUPR(r32i(spi)));
	v |= (1u << b32i(spi));
	mmio_w32(GICD_BASE + GICD_IGROUPR(r32i(spi)), v);

	// If IGRPMODR exists, ensure Non-secure Group-1 (bit=0)
	v = mmio_r32(GICD_BASE + GICD_IGRPMODR(r32i(spi)));
	v &= ~(1u << b32i(spi));
	mmio_w32(GICD_BASE + GICD_IGRPMODR(r32i(spi)), v);

	// Edge-triggered (simplest for “poke to signal”)
	v = mmio_r32(GICD_BASE + GICD_ICFGR(icfgr_i(spi)));
	v |= (2u << icfgr_sh(spi));              // 0b10 => edge
	mmio_w32(GICD_BASE + GICD_ICFGR(icfgr_i(spi)), v);

	// Priority
	*(volatile u8 *)(GICD_BASE + GICD_IPRIORITYR + prio_off(spi)) = 0x80;

	route_spi_to_this_cpu(spi);

	// Enable it
	mmio_w32(GICD_BASE + GICD_ISENABLER(r32i(spi)), (1u << b32i(spi)));
}
