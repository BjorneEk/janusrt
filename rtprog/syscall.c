
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#include "syscall.h"
#include "sched.h"
#include "timer.h"
#include "cpu.h"
#include "heap.h"
extern sched_t G_SCHED;
extern alloc_t G_ALLOC;

extern int G_VERB;

static void wait_until(u64 until)
{
	proc_t *p, *w;
	u64 dl;
	//bool sched_timer;

	uart_puts("WAIT\n");
	dump_sched(&G_SCHED, G_VERB);

	p = sched_yield(&G_SCHED);
	p->wait_until = until;
	p->state = PROC_WAITING;

	heap_peek(&G_SCHED.waiting, &dl, (void**)&w);

	//sched_timer = !sched_has_waiting(&G_SCHED);
	sched_wait_proc(&G_SCHED, p->pid);
	if (w == NULL || dl > until) {
		if (w)
			timer_cancel();
		timer_schedule_at_ticks(until);
	}
	uart_puts("WAIT after\n");
	dump_sched(&G_SCHED, G_VERB);
}

extern void jrt_exit(void);

static void exit()
{
	proc_t *p;

	uart_puts("before free\n");
	dump_alloc(&G_ALLOC);
	p = sched_yield(&G_SCHED);

	sched_free_proc(&G_SCHED, p->pid);
	uart_puts("after free\n");
	dump_alloc(&G_ALLOC);
}
static void spawn(u64 deadline, u64 ep, u64 ap, u64 mem_req)
{
	proc_t *p;
	u32 pid;
	void *mem;
	uart_puts("SPAWN \n");
	dump_sched(&G_SCHED, G_VERB);

	//interrupts_disable_all();
	mem = alloc(&G_ALLOC, mem_req);

	pid = sched_new_proc(
		&G_SCHED,
		G_SCHED.curr->pa_pc,
		G_SCHED.curr->prog_size,
		mem,
		mem_req,
		deadline,
		jrt_exit);
	p = sched_get_proc(&G_SCHED, pid);
	p->ctx.pc = ep;
	p->ctx.x[2] = ap;

	sched_ready_proc(&G_SCHED, pid);
	sched(&G_SCHED, sched_switch_irq);

	uart_puts("SPAWN after \n");
	dump_sched(&G_SCHED, G_VERB);
}

void take_syscall(u16 imm __attribute__((unused)))
{
	switch (G_SCHED.curr->ctx.x[8]) {
	case SYSCALL_WAIT_UNTIL:
		uart_puts("take syscall WAIT_UNTIL(");
		uart_putu64(G_SCHED.curr->pid);
		uart_puts(", ");
		uart_putu64(G_SCHED.curr->ctx.x[0]);
		uart_puts(") (now:");
		uart_putu64(time_now_ticks());
		uart_puts(")\n");
		wait_until(G_SCHED.curr->ctx.x[0]);
		break;
	case SYSCALL_EXIT:
		uart_puts("take syscall EXIT\n");
		exit();
		break;
	case SYSCALL_SPAWN:
		uart_puts("take syscall SPAWN(");
		uart_putu64(G_SCHED.curr->pid);
		uart_puts(", deadline: ");
		uart_putu64(G_SCHED.curr->ctx.x[0]);
		uart_puts(") (entry:");
		uart_puthex(G_SCHED.curr->ctx.x[1]);
		uart_puts(")\n");

		spawn(
			G_SCHED.curr->ctx.x[0],
			G_SCHED.curr->ctx.x[1],
			G_SCHED.curr->ctx.x[2],
			G_SCHED.curr->ctx.x[3]);
		break;
	}
}
static inline u64 invoke_syscall(u64 nr, u64 a0,u64 a1,u64 a2,u64 a3,u64 a4,u64 a5)
{
	register u64 x8 asm("x8") = nr;
	register u64 x0 asm("x0") = a0;
	register u64 x1 asm("x1") = a1;
	register u64 x2 asm("x2") = a2;
	register u64 x3 asm("x3") = a3;
	register u64 x4 asm("x4") = a4;
	register u64 x5 asm("x5") = a5;
	asm volatile("svc #0" : "+r"(x0)
			: "r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5),"r"(x8)
			: "memory","cc");
	return x0; // return value
}
void syscall(syscall_t call, ...)
{
	va_list va;
	u64 wait_until;

	va_start(va, call);
	switch (call) {
	case SYSCALL_WAIT_UNTIL:
		wait_until = va_arg(va, u64);
		uart_puts("invoke syscall WAIT_UNTIL(");
		uart_putu64(wait_until);
		uart_puts(")\n");
		invoke_syscall(SYSCALL_WAIT_UNTIL, wait_until, 0, 0, 0, 0, 0);
		break;
	case SYSCALL_EXIT:
		invoke_syscall(SYSCALL_EXIT, 0, 0, 0, 0, 0, 0);
		break;
	case SYSCALL_SPAWN:
		invoke_syscall(
			SYSCALL_SPAWN,
			va_arg(va, u64),
			va_arg(va, u64),
			va_arg(va, u64),
			va_arg(va, u64),
			0, 0);
		break;
	default:
		break;
	}
}
