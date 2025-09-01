
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _SCHED_H_
#define _SCHED_H_

#include "types.h"
#include "heap.h"
#include "mmu.h"

typedef enum task_state {
	PROC_READY,
	PROC_RUNNING,
	PROC_WAITING,
	PROC_UNUSED
} state_t;

typedef struct ctx {
	u64	x[31];
	u64	sp;
	u64	pc;
	u64	pstate;

	u128 vregs[32];
	u32 fpsr, fpcr;
	mmu_map_t mmap;
} ctx_t;

typedef struct process {
	u32	pid;
	state_t	state;
	int first;

	/* scheduling */
	u64	wait_until;      /* how long to wait */
	u64	abs_deadline;   /* base requested deadline */
	u64	eff_deadline;   /* effective (after inheritance) */
	u64	wait_start_ns;  /* for fairness */
	u64	wait_seq;       /* tiebreaker */

	ctx_t ctx;

	void *mem;
	size_t mem_size;

} proc_t;

#define READY_MAX (0xFF)
#define MAX_PROC (0x1000)

typedef struct sched {
	proc_t p0; //kernel proc
	proc_t pb[MAX_PROC];
	proc_t *free_proc[MAX_PROC];
	size_t nfree_proc;

	heap_node_t rn[READY_MAX];
	minheap_t ready;
	u32 pid;
} sched_t;

static inline s64 idx_to_pid(s64 i)
{
	return i + 1;
}

static inline s64 pid_to_idx(s64 p)
{
	return p - 1;
}

static inline void sched_init(sched_t *s)
{
	u32 i;

	for (i = 0; i < MAX_PROC; ++i) {
		s->pb[i].pid = idx_to_pid(i);
		s->pb[i].state = PROC_UNUSED;
		s->free_proc[i] = &s->pb[i];
	}
	s->nfree_proc = MAX_PROC;
	heap_init(&s->ready, s->rn, READY_MAX);
	s->pid = 0;
	s->p0.pid = 0;
	s->p0.first = 0;
	s->p0.ctx.mmap = kern_map_create(0);
}


proc_t *sched_get_proc(sched_t *sc, u32 pid);
proc_t *shed_alloc_proc(sched_t *sc);
void sched_sched_proc(sched_t *sc, u32 pid);

typedef void (*exit_func_t)(void);
u32 shed_new_proc(
	sched_t *sc,
	u64 pc,
	void *mem,
	size_t mem_size,
	u64 deadline,
	exit_func_t exit);

proc_t *sched_alloc_proc(sched_t *sc);

void sched_free_proc(sched_t *sc, u32 pid);

extern void store_pstate(ctx_t *ctx);
extern u64 load_pstate(ctx_t *ctx);

void sched(sched_t *sc);

#endif
