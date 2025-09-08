
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#include "sched.h"
#include "string.h"
#include "kerror.h"
#include "uart.h"
#include "alloc.h"
#include "timer.h"
extern alloc_t G_ALLOC;
proc_t *sched_alloc_proc(sched_t *sc)
{
	proc_t *r;

	if (sc->nfree_proc <= 0)
		return NULL;
	r = sc->free_proc[--sc->nfree_proc];
	return r;
}

proc_t *sched_get_proc(sched_t *sc, u32 pid)
{
	s64 idx;

	idx = pid_to_idx(pid);

	if (idx < 0 || idx >= MAX_PROC)
		KERNEL_PANIC(JRT_EINVAL);

	return &sc->pb[idx];
}

void sched_free_proc(sched_t *sc, u32 pid)
{
	proc_t *p;

	p = sched_get_proc(sc, pid);

	if (sc->nfree_proc >= MAX_PROC)
		KERNEL_PANIC(JRT_ENOMEM);

	p->state = PROC_UNUSED;
	sc->free_proc[sc->nfree_proc++] = p;
	free(&G_ALLOC, p->mem);
	mmu_map_destroy(&p->ctx.mmap);
}


u32 sched_new_proc(
	sched_t *sc,
	u64 pc,
	u64 code_size,
	void *mem,
	size_t mem_size,
	u64 deadline,
	exit_func_t exit)
{
	proc_t *p;
	static u16 asid = 1;

	p = sched_alloc_proc(sc);
	if (!p)
		KERNEL_PANIC(JRT_ENOMEM);

	//clear regs
	memset(p->ctx.x, 0, 31 * sizeof(u64));

	// stack pointer and procram counter
	p->ctx.sp = (uintptr_t)mem + mem_size;
	// align
	p->ctx.sp &= ~((uintptr_t)15);

	//mmu translates 0->pc
	p->ctx.pc = 0;
	//p->ctx.pc = pc;

	p->ctx.pstate = pstate_el1h(FIQ_MASK);

	// set default args for:
	// int start(void *mem, size_t mem_size);
	p->ctx.x[0] = (uintptr_t)mem;
	p->ctx.x[1] = mem_size;

	// final link
	p->ctx.x[30] = (uintptr_t)exit;

	// create mmap that maps code to 0:code_size
	p->ctx.mmap = proc_map_create(pc, code_size, asid++);

	p->pa_pc = pc;
	p->first = 1;
	p->mem = mem;
	p->mem_size = mem_size;
	p->prog_size = code_size;
	//p->code_size = code_size;
	p->eff_deadline = deadline;
	p->abs_deadline = deadline;
	p->state = PROC_READY;
	uart_puts("created proc, start addr: ");
	uart_puthex(pc);
	uart_puts("\nPC [0x0,0x50]: \n");
	dump_mem((void*)(uintptr_t)pc, 0x50);
	return p->pid;
}

void sched_ready_proc(sched_t *sc, u32 pid)
{
	proc_t *p;

	p = sched_get_proc(sc, pid);
	uart_puts("READY_PROC(");
	uart_putu32(pid);
	uart_puts(")\n");
	if (heap_push(&sc->ready, p->eff_deadline, p))
		KERNEL_PANIC(JRT_ENOMEM);
}

void sched_wait_proc(sched_t *sc, u32 pid)
{
	proc_t *p;

	p = sched_get_proc(sc, pid);
	uart_puts("WAIT_PROC(");
	uart_putu32(pid);
	uart_puts(")\n");

	if (heap_push(&sc->waiting, p->wait_until, p))
		KERNEL_PANIC(JRT_ENOMEM);
}
u64 sched_next_wait_deadline(sched_t *sc)
{
	proc_t *p;
	u64 wait_until;
	if (sched_has_waiting(sc))
		heap_peek(&sc->waiting, &wait_until, (void**)&p);
	else
		wait_until = 0;
	return wait_until;
}

void uart_dump_ctx(ctx_t *c)
{
	u32 i;
	uart_puts("pc: 0x");
	uart_puthex(c->pc);
	uart_puts("\n");

	uart_puts("sp: 0x");
	uart_puthex(c->sp);
	uart_puts("\n");

	uart_puts("pstate: 0x");
	uart_puthex(c->pstate);
	uart_puts("\n");
	for (i = 0; i < 31; ++i) {
		uart_puts("x");
		uart_putu32(i);
		uart_puts(": ");
		uart_putu64(c->x[i]);
		uart_puts(", ");
	}
	uart_puts("\n");
	uart_puts("asid: ");
	uart_putu32(c->mmap.asid);
	uart_puts("\n");

	dump_map(&c->mmap);
}

void sched_switch_sync(sched_t *sc, proc_t *c, proc_t *n)
{
	sc->pid = n->pid;
	store_pstate(&c->ctx);
	sc->curr = n;
	uart_puts("sync switch FROM:\n");

	uart_dump_ctx(&c->ctx);
	uart_puts("TO:\n");
	uart_dump_ctx(&n->ctx);
	mmu_map_switch(&n->ctx.mmap);
	load_pstate(&n->ctx);
}
void sched_switch_irq(sched_t *sc, proc_t *c, proc_t *n)
{
	sc->pid = n->pid;
	sc->curr = n;
	uart_putu32(c->pid);
	uart_puts(", pc: (");
	uart_puthex(c->ctx.pc);
	uart_puts(", PA: ");
	uart_puthex(c->pa_pc);
	uart_puts(") -> (pid: ");
	uart_putu32(n->pid);
	uart_puts(", pc: (");
	uart_puthex(n->ctx.pc);
	uart_puts(", PA: ");
	uart_puthex(n->pa_pc);
	uart_puts(")\n");
	/*
	uart_puts("irq switch FROM:\n");
	uart_dump_ctx(&c->ctx);
	uart_puts("TO:\n");
	uart_dump_ctx(&n->ctx);
	uart_puts("PC (PA): ");
	uart_puthex(n->pa_pc);
	uart_puts("\nPC [0x0,0x50]: \n");
	dump_mem((void*)(uintptr_t)n->pa_pc, 0x50);
	*/

}
proc_t *sched_yield(sched_t *sc)
{
	proc_t *p, *c;
	u64 deadline;

	heap_pop(&sc->ready, &deadline, (void**)&p);
	if (!p)
		p = &sc->p0;
	p->state = PROC_RUNNING;
	c = sc->pid == 0 ? &sc->p0 : sched_get_proc(sc, sc->pid);
	uart_puts("[");
	uart_putu64(time_now_us());
	uart_puts("][SCHED] yield (pid: ");
	sched_switch_irq(sc, c, p);
	return c;
}

void sched(sched_t *sc, void (*swp)(sched_t*,proc_t*,proc_t*))
{
	proc_t *p, *c;
	u64 deadline;

	heap_peek(&sc->ready, &deadline, (void**)&p);
	if (!p) {
		if (sc->curr == NULL)

		return;
	}
	c = sc->curr->pid == 0 ? &sc->p0 : sched_get_proc(sc, sc->pid);

	if (sc->curr->pid == 0 || c->eff_deadline > deadline) {
		// should perform context switch
		heap_pop(&sc->ready, &deadline, (void**)&p);
		p->state = PROC_RUNNING;
		c->state = PROC_READY;
		if (sc->curr->pid != 0)
			sched_ready_proc(sc, c->pid);
		uart_puts("[");
		uart_putu64(time_now_us());
		uart_puts("][SCHED] switch (pid: ");

		swp(sc, c, p);
	}
}
/*
void sched(sched_t *sc)
{
	proc_t *p, *c;
	u64 deadline;

	heap_peek(&sc->ready, &deadline, (void**)&p);
	if (!p)
		return;

	c = sc->pid == 0 ? &sc->p0 : sched_get_proc(sc, sc->pid);

	if (sc->pid == 0 || c->eff_deadline > deadline) {
		// should perform context switch
		heap_pop(&sc->ready, &deadline, (void**)&p);
		p->state = PROC_RUNNING;
		c->state = PROC_READY;
		if (sc->pid != 0)
			sched_sched_proc(sc, c->pid);
		sc->pid = p->pid;
		store_pstate(&c->ctx);
		sc->curr = p;
		uart_puts("switch FROM:\n");
		uart_dump_ctx(&c->ctx);
		uart_puts("TO:\n");
		uart_dump_ctx(&p->ctx);
		mmu_map_switch(&p->ctx.mmap);
		load_pstate(&p->ctx);
	}
}
*/
