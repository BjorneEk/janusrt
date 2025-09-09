
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _SCHED_H_
#define _SCHED_H_

#include "types.h"
#include "sched_structs.h"
#include "heap.h"
#include "mmu.h"
// PSTATE / SPSR bits used here
#define PSR_F   (1u << 6)   // FIQ mask
#define PSR_I   (1u << 7)   // IRQ mask
#define PSR_A   (1u << 8)   // SError mask
#define PSR_D   (1u << 9)   // Debug mask

#define IRQ_MASK	(PSR_I)
#define FIQ_MASK	(PSR_F)
#define SERR_MASK	(PSR_A)
#define DBG_MASK	(PSR_D)
// M[3:0] encodings for AArch64
#define PSR_M_EL0t  0x0u
#define PSR_M_EL1t  0x4u
#define PSR_M_EL1h  0x5u

static inline u64 pstate_el1h(u64 ex_mask)
{
	u64 p;
	p = PSR_M_EL1h;
	p |= ex_mask;
	return p;
}
static inline u64 pstate_el1t(u64 ex_mask)
{
	u64 p;
	p = PSR_M_EL1t;
	p |= ex_mask;
	return p;
}


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
	heap_init(&s->waiting, s->wn, READY_MAX);

	s->pid = 0;
	s->p0.pid = 0;
	s->p0.first = 0;
	s->p0.ctx.mmap = kern_map_create(0);
	s->curr = &s->p0;
}


proc_t *sched_get_proc(sched_t *sc, u32 pid);
proc_t *shed_alloc_proc(sched_t *sc);
void sched_ready_proc(sched_t *sc, u32 pid);

void sched_wait_proc(sched_t *sc, u32 pid);
static inline bool sched_has_waiting(sched_t *sc)
{
	return sc->waiting.len > 0;
}
u64 sched_next_wait_deadline(sched_t *sc);


typedef void (*exit_func_t)(void);
u32 sched_new_proc(
	sched_t *sc,
	u64 pc,
	u64 code_size,
	void *mem,
	size_t mem_size,
	u64 deadline,
	exit_func_t exit);

proc_t *sched_alloc_proc(sched_t *sc);

void sched_free_proc(sched_t *sc, u32 pid);

extern void store_pstate(ctx_t *ctx);
extern u64 load_pstate(ctx_t *ctx);

void dump_proc(proc_t *p, int v);

void dump_sched(sched_t *s, int v);
// ctx_switch.S
void update_current_ctx(void);

proc_t *sched_yield(sched_t *sc);
void sched(sched_t *sc, void (*swp)(sched_t*,proc_t*,proc_t*));
void sched_switch_sync(sched_t *sc, proc_t *c, proc_t *n);
void sched_switch_irq(sched_t *sc, proc_t *c, proc_t *n);
void uart_dump_ctx(ctx_t *c);
#endif
