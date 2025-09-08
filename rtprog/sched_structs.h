
// Author: Gustaf Franzen <gustaffranzen@icloud.com>

#ifndef _SCHED_STRUCTS_H_
#define _SCHED_STRUCTS_H_
#include "types.h"
#include "mmu_structs.h"
#include "heap_structs.h"

typedef enum task_state {
	PROC_READY,
	PROC_RUNNING,
	PROC_WAITING,
	PROC_UNUSED
} state_t;

typedef  struct ctx {
	u64	x[31];
	u64	sp;
	u64	pc;
	u64	pstate;

	u128 vregs[32];
	u32 fpsr, fpcr;
	mmu_map_t mmap;
} ctx_t;

typedef struct process {
	ctx_t ctx;
	u32	pid;
	state_t	state;
	int first;
	u64 pa_pc;
	/* scheduling */
	u64	wait_until;      /* how long to wait */
	u64	abs_deadline;   /* base requested deadline */
	u64	eff_deadline;   /* effective (after inheritance) */
	u64	wait_start_ns;  /* for fairness */
	u64	wait_seq;       /* tiebreaker */

	u64 prog_size;
	void *mem;
	size_t mem_size;
} proc_t;

#define READY_MAX (0xFF)
#define MAX_PROC (0x1000)

typedef struct sched {
	proc_t p0; //kernel proc
	proc_t *curr;
	u32 pid;
	proc_t pb[MAX_PROC];
	proc_t *free_proc[MAX_PROC];
	size_t nfree_proc;

	heap_node_t rn[READY_MAX];
	minheap_t ready;
	heap_node_t wn[READY_MAX];
	minheap_t waiting;
} sched_t;



#endif
