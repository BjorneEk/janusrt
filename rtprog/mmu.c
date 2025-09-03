
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
	u64 base;
	unsigned i;
	// Distributor
	map_range_pages(r, GICD_PA_BASE, GICD_PA_BASE, GICD_PA_SIZE, PTE_DEV_RW_G);

	// Redistributors: one 128 KiB frame per CPU starting at GICR_PA_BASE0
	for (i = 0; i < GICR_NUM_CPUS; ++i) {
		base = GICR_PA_BASE0 + (u64)i * GICR_FRAME_SIZE;
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
	//map_range_pages(r, 0, 0, CODE_0, PTE_KERN_RWX);
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
	//
	//uart_puts("[MMU] switch:\n");
	//dump_map(pm);
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



static inline u64 *root_table_va(const mmu_map_t *m)
{
	// Prefer VA pointer if set, else assume identity for PA.
	return m->root.l0 ? m->root.l0 : (u64 *)(uintptr_t)m->root.l0_pa;
}

/* ---- descriptor helpers for 4K granule ---- */

static inline int desc_kind(u64 d, int level)
{
	u64 low2;
	// return: 0=invalid, 1=table, 2=block_or_page
	if ((d & PTE_VALID) == 0)
		return 0;
	low2 = d & 0x3ULL; // bits [1:0]
	if (level < 3) {
		if (low2 == DESC_TABLE)
			return 1;          // next-level table
		if (low2 == DESC_BLOCK && (level == 1 || level == 2))
			return 2; // block
		return 0; // reserved form at L0/invalid
	} else {
		// L3: valid page has DESC_PAGE (==3)
		return (low2 == DESC_PAGE) ? 2 : 0;
	}
}

static inline u64 outaddr_mask(void)
{
	u64 pa_field_mask;

	// keep PA_BITS and clear low PAGE_SHIFT bits
	pa_field_mask = (PA_BITS >= 64) ? ~0ULL : ((1ULL << PA_BITS) - 1ULL);
	return pa_field_mask & PAGE_MASK;
}

static inline u64 desc_outaddr(u64 d)
{
	return d & outaddr_mask();
}

static inline u64 level_span_bytes(int level)
{
	switch (level) {
		case 0: return 1ULL << 39; // 512 GiB per L0 slot (4K granule)
		case 1: return 1ULL << 30; // 1 GiB per L1 block
		case 2: return 1ULL << 21; // 2 MiB per L2 block
		case 3: return 1ULL << 12; // 4 KiB per L3 page
		default: return 0;
	}
}

/* ---- simple coalescer so output is readable ---- */

static struct {
	bool active;
	u64 va, pa, len;
} run_;

static inline void run_flush(void)
{
	if (!run_.active)
		return;
	pr_map(run_.va, run_.pa, run_.len);
	run_.active = false;
}

static inline void run_visit(u64 va, u64 pa, u64 size)
{
	if (run_.active &&
			run_.va + run_.len == va &&
			run_.pa + run_.len == pa) {
		run_.len += size;
		return;
	}
	run_flush();
	run_.active = true;
	run_.va  = va;
	run_.pa  = pa;
	run_.len = size;
}


static void putrow(
	char b1[3*8+2],
	char b2[3*8+2],
	char b1c[3*8+2],
	char b2c[3*8+2])
{
	uart_puts("|");
	uart_puts(b1);
	uart_puts("|");
	uart_puts(b2);
	uart_puts("|");
	uart_puts(b1c);
	uart_puts("|");
	uart_puts(b2c);
	uart_puts("|\n");
}
static void str_putu64(char *buff, u64 v, u32 n)
{
	int i;

	i = n;
	//buf[i--] = '\0';

	if (v == 0) {
		*buff = '0';
		return;
	}

	while (v && i >= 0) {
		buff[i--] = '0' + (v % 10);
		v /= 10;
	}
	return;
}
static char tohex(int i)
{
	if (i < 10)
		return '0' + i;
	return 'A' + (i - 10);
}

static void str_putaddr(char *buff, u64 v, u32 n)
{
	int i;

	i = n;

	while (v && i >= 0) {
		buff[i--] = v == 0 ? '0' : tohex(v % 16);
		if (v == 0)
			return;
		v /= 16;
	}
	return;
}

static void str_butb(char *buff, u32 v)
{
	u32 nib;
	nib = (v >> 4) & 0xFULL;
	buff[0] = nib < 10 ? ('0' + nib) : ('a' + (nib - 10));
	nib = v & 0xFULL;
	buff[1] = nib < 10 ? ('0' + nib) : ('a' + (nib - 10));
}

void dump_mem(const void *start_addr, u64 n)
{
	const u8 *d;
	u64 i, j, r, cnt;
	const u8 rl = 8*3+1;
	char a[rl+1];
	char b1[rl+1];
	char b2[rl+1];
	char b1c[rl+1];
	char b2c[rl+1];
	char *b;
	a[rl - (8*2 - 2)] = '\0';
	b1[rl] = '\0';
	b2[rl] = '\0';
	b1c[rl - (8*2 - 2)] = '\0';
	b2c[rl - (8*2 - 2)] = '\0';
	d = start_addr;

	for (i = r = cnt = 0; i < n; i += 0x10) {
		memset(a, ' ', rl - (8*2 - 1));
		memset(b1, ' ', rl);
		memset(b2, ' ', rl);
		memset(b1c, ' ', rl - (8*2 - 1));
		memset(b2c, ' ', rl - (8*2 - 1));
		if(i != 0 && !memcmp(&d[i], &d[i - 0x10], 0x10)) {
			r = 1;
			cnt++;
			continue;
		}

		if (r) {
			a[1] = '*';
			str_putu64(&a[2], cnt, 6);
			uart_puts("|");
			uart_puts(a);
			putrow(b1, b2, b1c, b2c);
			r = cnt = 0;
		}

		str_putaddr(&a[1], (uintptr_t)(&d[i]), 7);
		for (j = 0; j < 0x10; ++j) {
			b = j < 0x8 ? &b1[1+j*3] : &b2[1+(j - 0x8)*3];
			str_butb(b, d[i + j]);
		}
		for (j = 0; j < 0x10; ++j) {
			b = j < 0x8 ? &b1c[1+j] : &b2c[(1+j - 0x8)];

			b[0] = (31 < d[i+j] && 127 > d[i+j]) ?
				d[i+j] : '*';
		}
		uart_puts("|");
		uart_puts(a);
		putrow(b1, b2, b1c, b2c);
	}
}
#define ENTRIES_PER_TABLE 512

static void walk_level(int level, u64 *table, u64 va_base)
{
	const u64 slot_span = level_span_bytes(level);
	u64	va_slot,
		next_pa,
		*next,
		size,
		pa,
		d;
	int i, k;

	for (i = 0; i < ENTRIES_PER_TABLE; ++i) {
		d = table[i];
		k = desc_kind(d, level);
		if (k == 0)
			continue;

		va_slot = va_base + (u64)i * slot_span;

		if (k == 1) {
			// next level table
			next_pa = desc_outaddr(d);
			// assume PT memory is visible at this VA
			next = (u64 *)(uintptr_t)next_pa;
			walk_level(level + 1, next, va_slot);
		} else { // block or page
			size = slot_span;
			pa   = desc_outaddr(d);
			run_visit(va_slot, pa, size);
		}
	}
}


void dump_map(const mmu_map_t *map)
{
	u64 *l0;

	l0 = root_table_va(map);
	if (!l0) {
		uart_puts("[MMU]: <no L0>\n");
		return;
	}
	run_.active = false;
	walk_level(0, l0, 0);
	run_flush();
}
