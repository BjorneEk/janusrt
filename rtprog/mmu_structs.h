
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _MMU_STRUCTS_H_
#define _MMU_STRUCTS_H_
#include "types.h"

typedef struct {
	u64 *l0;     // root (must be 4 KiB aligned)
	u64  l0_pa;  // physical (early: VA==PA)
} pt_root_t;

typedef struct {
	pt_root_t root;
	u16  asid;
} mmu_map_t;


#endif
