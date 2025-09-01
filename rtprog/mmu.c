
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#include "mmu.h"
#include "alloc.h"
#include "string.h"
#include "kerror.h"
#include "uart.h"
static alloc_t *MMU_ALLOC = NULL;

void mmu_set_alloc(alloc_t *a)
{
	MMU_ALLOC = a;
}

// ---------- PT allocation via your allocator ----------
static inline void *pt_alloc_page_or_die(void)
{
	void *p;
	u64 align;

	p = aligned_alloc(MMU_ALLOC, PAGE_SIZE, PAGE_SIZE);
	KASSERT(p);

	align = ((uintptr_t)p & (PAGE_SIZE - 1));
	KASSERT(align == 0);

	memset(p, 0, PAGE_SIZE);
	return p;
}

// ---------- MAIR/TCR ----------
u64 make_mair_el1(void)
{
	u64 mair;
	// AttrIdx0 = Normal WB WA RA (0xFF),
	// AttrIdx1 = Device-nGnRE (0x04) (reserved for MMIO)
	mair = 0;
	mair |= 0xFFULL << (8 * MAIR_IDX_NORMAL);
	mair |= 0x04ULL << (8 * MAIR_IDX_DEVICE);
	return mair;
}

static inline u64 tcr_ips_enc(unsigned pa_bits)
{
	switch (pa_bits) {
	case 32: return 0ULL;
	case 36: return 1ULL;
	case 40: return 2ULL;
	case 42: return 3ULL;
	case 44: return 4ULL;
	case 48: return 5ULL;
	case 52: return 6ULL;
	default: return 1ULL;
	}
}

u64 make_tcr_el1(unsigned va_bits, unsigned pa_bits, bool enable_ttbr1)
{
	u64 tcr;
	unsigned t0sz, t1sz;

	tcr = 0;
	t0sz = 64u - va_bits;
	t1sz = 64u - va_bits;

	// TTBR0 (low VA)
	tcr |= (u64)t0sz;	// T0SZ
	tcr |= (0ULL << 7);	// EPD0=0 (enable)
	tcr |= (1ULL << 8);	// IRGN0=01 (WB WA)
	tcr |= (1ULL << 10);	// ORGN0=01
	tcr |= (3ULL << 12);	// SH0=11 (Inner)
	tcr |= (0ULL << 14);	// TG0=00 (4 KiB)

	// TTBR1 (high VA) — disabled
	tcr |= ((u64)t1sz << 16);
	tcr |= (0ULL << 22);	// A1=0 (ASID from TTBR0)
	tcr |= ((enable_ttbr1 ? 0ULL : 1ULL) << 23); // EPD1
	tcr |= (1ULL << 24);	// IRGN1=01
	tcr |= (1ULL << 26);	// ORGN1=01
	tcr |= (3ULL << 28);	// SH1=11
	tcr |= (1ULL << 30);	// TG1=01 (4 KiB)

	tcr |= (tcr_ips_enc(pa_bits) << 32); // IPS
	return tcr;
}




// ---------- Indexing ----------
static inline u64 idx_l0(u64 va){ return (va >> 39) & 0x1FF; }
static inline u64 idx_l1(u64 va){ return (va >> 30) & 0x1FF; }
static inline u64 idx_l2(u64 va){ return (va >> 21) & 0x1FF; }
static inline u64 idx_l3(u64 va){ return (va >> 12) & 0x1FF; }

static inline u64 desc_table(u64 pa)
{
	return (pa & ~0xFFFULL) | PTE_VALID | DESC_TABLE;
}

static inline u64 desc_page(u64 pa, u64 attrs)
{
	return (pa & ~0xFFFULL) | attrs;
}


static inline bool valid(u64 d) { return (d & PTE_VALID) != 0; }

// descend/allocate
static u64 *ensure_next(u64 *lvl, u64 idx)
{
	u64 d;
	void *p;
	d = lvl[idx];
	if (!valid(d)) {
		p = pt_alloc_page_or_die();
		lvl[idx] = desc_table((u64)p);
		return (u64*)p;
	}
	return (u64*)(d & ~0xFFFULL);
}



// ---------- Roots & mappers ----------
pt_root_t pt_root_new(void)
{
	void *l0;
	l0 = pt_alloc_page_or_die();
	pt_root_t r = { .l0 = (u64*)l0, .l0_pa = (u64)l0 };
	return r;
}

void map_page(pt_root_t r, u64 va, u64 pa, u64 attrs)
{
	u64	*l1,
		*l2,
		*l3;

	l1 = ensure_next(r.l0, idx_l0(va));
	l2 = ensure_next(l1,   idx_l1(va));
	l3 = ensure_next(l2,   idx_l2(va));
	l3[idx_l3(va)] = desc_page(pa, attrs);
}
static void pr_map(u64 va, u64 pa, u64 len)
{
	uart_puts("[MMU]: map PA [");
	uart_puthex(pa);
	uart_puts(", ");
	uart_puthex(pa + len);
	uart_puts(") -> [");
	uart_puthex(va);
	uart_puts(", ");
	uart_puthex(va + len);
	uart_puts(")\n");
}
void map_range_pages(pt_root_t r, u64 va, u64 pa, u64 len, u64 attrs)
{
	u64 end;
	pr_map(va, pa, len);
	end = va + len;
	va  &= PAGE_MASK;
	pa  &= PAGE_MASK;
	end  = (end + PAGE_SIZE - 1) & PAGE_MASK;
	for (; va < end; va += PAGE_SIZE, pa += PAGE_SIZE)
		map_page(r, va, pa, attrs);
}
static inline void map_uart(pt_root_t r)
{
	map_range_pages(r, UART_PA_BASE, UART_PA_BASE, UART_PA_SIZE, PTE_DEV_RW_G);
}

static inline void map_gic(pt_root_t r)
{
	// Distributor
	map_range_pages(r, GICD_PA_BASE, GICD_PA_BASE, GICD_PA_SIZE, PTE_DEV_RW_G);

	// Redistributors: one 128 KiB frame per CPU starting at GICR_PA_BASE0
	for (unsigned i = 0; i < GICR_NUM_CPUS; ++i) {
		uint64_t base = GICR_PA_BASE0 + (uint64_t)i * GICR_FRAME_SIZE;
		map_range_pages(r, base, base, GICR_FRAME_SIZE, PTE_DEV_RW_G);
	}
}
// ---------- Builders ----------

// Per-process map:
//   [0, CODE_0)  →  [proc_base, proc_base+CODE_0)   (RWX, nG=1)
//   [CODE_0, __kernel_end) → identity (RWX, global)
//   [MEM_0, MEM_END]       → identity (RWX, global)
//   Everything else unmapped (so other procs aren't visible).
void build_process_map(pt_root_t r, u64 proc_base_pa, u64 proc_image_len)
{
	u64 low_len, k_end;

	low_len = CODE_0;
	if (proc_image_len < low_len)
		low_len = proc_image_len;

	// Low window (per-proc)
	map_range_pages(r, 0, proc_base_pa, low_len, PTE_PROC_RWX);

	// Kernel identity
	k_end = (u64)__kernel_end;
	if (k_end > CODE_0)
		map_range_pages(r, CODE_0, CODE_0, k_end - CODE_0, PTE_KERN_RWX);

	// Allocator identity
	map_range_pages(r, MEM_0, MEM_0, (MEM_END + 1ULL) - MEM_0, PTE_KERN_RWX);

	map_uart(r);
}

// Kernel master map (used only while running kernel):
//   Identity map EVERYTHING the kernel may touch: [CODE_0 .. MEM_END] (RWX, global).
void build_kernel_map(pt_root_t r)
{
	map_range_pages(r, CODE_0, CODE_0, (MEM_END + 1ULL) - CODE_0, PTE_KERN_RWX);

	map_uart(r);
	map_gic(r);
}


// ---------- Map handles ----------
typedef struct { pt_root_t root; u16 asid; } _map_any_t;

mmu_map_t proc_map_create(u64 proc_base_pa, u64 image_len, u16 asid)
{
	pt_root_t r;
	mmu_map_t pm;
	r = pt_root_new();
	build_process_map(r, proc_base_pa, image_len);
	pm = (mmu_map_t){ .root = r, .asid = asid };
	return pm;
}

mmu_map_t kern_map_create(u16 asid)
{
	pt_root_t r;
	mmu_map_t km;

	r = pt_root_new();
	build_kernel_map(r);
	km = (mmu_map_t){ .root = r, .asid = asid };
	return km;
}
// ---- internal walker: free table pages, never data frames ----
// level: 0=L0, 1=L1, 2=L2, 3=L3
static void free_table_level(u64 *table, int level)
{
	int i;
	u64 d, type, *next;
	// Walk entries
	for ( i = 0; i < 512; ++i) {
		d = table[i];
		if (!(d & PTE_VALID))
			continue;

		type = d & 0x3;  // [1:0]
		if (level < 3) {
			if (type == DESC_TABLE) {
				next = (u64*)(d & ~0xFFFULL); // next-level base
				free_table_level(next, level + 1);
			}
			// else: DESC_BLOCK (1 GiB/2 MiB)
			// — we never emit blocks in our mapper,
			// so nothing to free here.
		} else {
			// level == 3: leaf PAGE entries point to frames; do not free.
		}
		table[i] = 0; // scrub entry (optional)
	}

	// Free this table page itself
	free(MMU_ALLOC, table);
}

void pt_root_free(pt_root_t r)
{
	if (!r.l0)
		return;
	free_table_level(r.l0, 0);
}

void mmu_map_switch(const mmu_map_t *pm)
{
	// If reusing ASIDs, TLBI that ASID first:
	// tlbi_asid(pm->asid); dsb ish; isb();
	write_ttbr0_asid(pm->root.l0_pa, pm->asid);
	// global kernel entries remain valid; ISB suffices.
}

void mmu_map_destroy(mmu_map_t *km)
{
	if (!km)
		return;
	pt_root_free(km->root);
	km->root.l0 = NULL;
	km->root.l0_pa = 0;
	km->asid = 0;
}

static u64 G_MAIR;
static u64 G_TCR;

void mmu_enable(mmu_map_t *m)
{
	G_MAIR = make_mair_el1();
	G_TCR = make_tcr_el1(VA_BITS, PA_BITS, /*enable_ttbr1=*/false);
	el1_mmu_on(G_MAIR, G_TCR, m->root.l0_pa);
}




