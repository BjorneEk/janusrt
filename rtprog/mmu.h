
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _MMU_H_
#define _MMU_H_
#include "memory_layout.h"
#include "types.h"
#include "alloc.h"
#include "uart.h"

extern char __kernel_start[];
extern char __kernel_end[];
// ==== VA/PA & granule policy ====
#define VA_BITS          48u          // 48-bit VA space (TTBR0)
#define PA_BITS          36u          // adjust to your SoC (36..48 typical)

#define PAGE_SHIFT       12u
#define PAGE_SIZE        (1UL << PAGE_SHIFT)
#define PAGE_MASK        (~(PAGE_SIZE - 1ULL))

#define CODE_0           JRT_CODE_PHYS
#define CODE_END         (JRT_CODE_PHYS + (JRT_CODE_SIZE - 1))
#define MEM_0            JRT_MEM_PHYS
#define MEM_END          (JRT_MEM_PHYS + (JRT_MEM_SIZE - 1))

// ===== UART MMIO
#define UART_PA_BASE     UART_BASE
#define UART_PA_SIZE     0x1000ULL

// ===== GICv3 MMIO (QEMU virt) =====
#define GICD_PA_BASE      0x08000000ULL
#define GICD_PA_SIZE      0x00010000ULL   // 64 KiB distributor

#define GICR_PA_BASE0     0x080A0000ULL   // first redistributor frame
#define GICR_FRAME_SIZE   0x00020000ULL   // 128 KiB per CPU (RD + SGI/PPI)
#ifndef GICR_NUM_CPUS
#define GICR_NUM_CPUS     4               // set to your CPU count
#endif
// ==== PTE / descriptor bits (AArch64, 4 KiB) ====

// [1:0]
#define DESC_INVALID     0ULL
#define DESC_BLOCK       1ULL
#define DESC_TABLE       3ULL
#define DESC_PAGE        3ULL


#define PTE_UXN		(1ULL << 54)// EL0 execute-never(safe to set even if EL0 off)
#define PTE_PXN		(1ULL << 53)// EL1 execute-never(LEAVE THIS CLEARfor EL1 exec)
#define PTE_CONTIG	(1ULL << 52)
#define PTE_DBM		(1ULL << 51)
#define PTE_nG		(1ULL << 11) // not-Global (set for per-process entries)
#define PTE_AF		(1ULL << 10)
#define PTE_SH_NON	(0ULL << 8)
#define PTE_SH_OUTER	(2ULL << 8)
#define PTE_SH_INNER	(3ULL << 8)
#define PTE_AP_RW_EL1	(0ULL << 6)       // EL1 RW, EL0 no access
#define PTE_AP_RO_EL1	(2ULL << 6)
#define PTE_ATTRIDX(i)	((uint64_t)(i) << 2)
#define PTE_VALID        (1ULL << 0)

// MAIR indices
#define MAIR_IDX_NORMAL  0u
#define MAIR_IDX_DEVICE  1u

// ---------- RWX presets ----------
// Kernel/master entries: GLOBAL (nG=0)
#define PTE_KERN_RWX (PTE_VALID|DESC_PAGE|PTE_AF|PTE_SH_INNER| \
		PTE_AP_RW_EL1|PTE_ATTRIDX(MAIR_IDX_NORMAL)|\
		PTE_UXN /*PXN=0→EL1 exec*/)

// Per-process entries: NON-GLOBAL (nG=1) so ASID works properly
#define PTE_PROC_RWX (PTE_VALID|DESC_PAGE|PTE_AF|PTE_SH_INNER| \
			PTE_AP_RW_EL1|PTE_ATTRIDX(MAIR_IDX_NORMAL) | \
			PTE_UXN|PTE_nG)

#define PTE_DEV_RW_G  (PTE_VALID | DESC_PAGE | PTE_AF | PTE_SH_OUTER | \
			PTE_AP_RW_EL1 | PTE_ATTRIDX(MAIR_IDX_DEVICE) | \
			PTE_UXN | PTE_PXN /* nG=0 → global */)

void mmu_set_alloc(alloc_t *a);

// ==== MAIR/TCR helpers ====
u64 make_mair_el1(void);
u64 make_tcr_el1(unsigned va_bits, unsigned pa_bits, bool enable_ttbr1);

// ==== Page-table root & API ====
typedef struct {
	u64 *l0;     // root (must be 4 KiB aligned)
	u64  l0_pa;  // physical (early: VA==PA)
} pt_root_t;

pt_root_t pt_root_new(void);  // alloc & zero L0
void      map_page(pt_root_t r, u64 va, u64 pa, u64 attrs);
void      map_range_pages(pt_root_t r, u64 va, u64 pa, u64 len, u64 attrs);

// ==== High-level builders ====
void build_kernel_map(pt_root_t r);                       // identity map kernel + allocator, RWX
void build_process_map(pt_root_t r, u64 proc_base_pa, u64 proc_image_len);

// ==== EL1 MMU control ====
void el1_mmu_on(u64 mair, u64 tcr, u64 ttbr0_pa);
void write_ttbr0_asid(u64 ttbr0_pa, u16 asid);
void tlbi_all(void);
void tlbi_asid(u16 asid);

// ==== Process map handle & switch ====
typedef struct {
	pt_root_t root;
	u16  asid;
} mmu_map_t;

mmu_map_t proc_map_create(u64 proc_base_pa, u64 image_len, u16 asid);
mmu_map_t kern_map_create(u16 asid);
void mmu_map_switch(const mmu_map_t *pm);


void mmu_enable(mmu_map_t *m);

// Free an entire page-table tree (must not be active in TTBR0 when called)
void pt_root_free(pt_root_t r);

// Destroy helpers
void mmu_map_destroy(mmu_map_t *pm);
#endif
