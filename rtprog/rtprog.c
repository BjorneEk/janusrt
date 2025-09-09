/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */

// core includes
#include <stdint.h>

// shared includes
#include "mailbox.h"
#include "rtcore.h"
#include "memory_layout.h"

// libc includes
#include "string.h"

// jrt includes
#include "timer.h"
#include "irq.h"
#include "uart.h"
#include "cpu.h"
#include "kerror.h"
#include "sched.h"
#include "alloc.h"
#include "gic.h"
#include "syscall.h"

sched_t G_SCHED;
alloc_t G_ALLOC;
ctx_t *G_KERNEL_CTX;

int G_VERB = 2;

void ipc_sched(ctx_t *);
void timer_fn(ctx_t *);
//static void trampoline(ctx_t *c)
//{
//	timer_irq_handler(c);
//}

void timer_ppi_init(void)
{
	timebase_init();
	irq_register_ppi(EL1_PHYS_TIMER_PPI, timer_fn);
	gic_enable_ppi(EL1_PHYS_TIMER_PPI, 0x80, /*group1ns=*/1);
	asm volatile("msr DAIFClr, #2\n\tisb" ::: "memory");
}
void sched_spi_init(void)
{
	irq_register_spi(SCHED_SPI, ipc_sched);
	sys_enable_icc_el1();
	gic_enable_dist();
	gic_enable_spi(SCHED_SPI);
}
/*
 * recovery function for synchronous exceptions
 */
static void sync_recover(enum panic_reason r)
{
	uart_puts("[JRT RECOVERY] ");
	uart_puts(panic_str(r));
	uart_puts("\n");
	if (r == USER_PANIC)
		return;
	uart_puts("reason: ");
	uart_puts(jrt_strerror());
	uart_puts("\n");
	if (jrt_errno() == JRT_EASSERT)
		jrt_print_assert();
	uart_puts("\n");
	halt();
}

static void jrt_loop(void)
{
	uart_puts("kernel spin\n");
	for (;;) {
		wfe();
	}
}

void jrt_main(void)
{
	uint32_t i;

	interrupts_disable_all();
	// initialize UART
	uart_init();

	// set handler for syncronous exceptions
	sync_set_recover_handler(sync_recover);

	// initialize irq handlers
	irq_init();

	//initialize allocator
	alloc_init(&G_ALLOC, (void*)(uintptr_t)JRT_HEAP_START, JRT_HEAP_SIZE);

	//set allocator used by mmu
	mmu_set_alloc(&G_ALLOC);

	// initialize scheduler (also creates kernel mmap)
	sched_init(&G_SCHED);
	G_KERNEL_CTX = &G_SCHED.p0.ctx;
	uart_puts("&G_SCHED: ");
	uart_puthex((uintptr_t)&G_SCHED);
	uart_puts("\n&G_SCHED.curr: ");
	uart_puthex((uintptr_t)&G_SCHED.curr);
	uart_puts("\nG_SCHED.curr: ");
	uart_puthex((uintptr_t)G_SCHED.curr);
	uart_puts("\n&G_SCHED.curr->ctx: ");
	uart_puthex((uintptr_t)&G_SCHED.curr->ctx);
	uart_puts("\n");
	// enable mmu with kernel (linear) mmap
	mmu_enable(&G_SCHED.p0.ctx.mmap);

	interrupts_enable_all();

	// initialize periodix timer
	//timer_init();
	sched_spi_init();
	timer_ppi_init();
	jrt_loop();

	uart_puts("a number: ");
	uart_putu32(69);
	uart_puts("\n");

	for (i = 0; i < 100; ++i) {
		uart_puts("[jrt] Hello from JRT! ");
		uart_putu32(i);
		uart_puts("\n");
	}
}

static inline int mpsc_pop(struct mpsc_ring *r, u8 out[TOJRT_REC_SIZE])
{
	u32 tail;
	u32 idx;

	tail = r->tail;
	idx = tail & TOJRT_MASK;

	if (!__atomic_load_n(&r->flags[idx], __ATOMIC_ACQUIRE))
		return -11;

	memcpy(out, (u8*)&r->data[idx], sizeof(jrt_sched_req_t));

	__atomic_store_n(&r->flags[idx], 0u, __ATOMIC_RELEASE);
	r->tail = tail + 1;
	return 0;
}
void jrt_exit(void)
{
	syscall(SYSCALL_EXIT);
}
void schedule_req(u64 pc, u64 prog_size, u64 mem_req)
{
	u32 pid;
	void *mem;

	//interrupts_disable_all();
	mem = alloc(&G_ALLOC, mem_req);
	uart_puts("SCHED\n");
	dump_sched(&G_SCHED, G_VERB);

	if (!mem)
		KERNEL_PANIC(JRT_ENOMEM);

	pid = sched_new_proc(
		&G_SCHED,
		pc,
		prog_size,
		mem,
		mem_req,
		0,
		jrt_exit);
	uart_puts("[SCHED] ");
	uart_putu32(pid);
	uart_puts(": (");
	uart_putu64(pc);
	uart_puts(", ");
	uart_putu64(prog_size);
	uart_puts(", ");
	uart_putu64(mem_req);
	uart_puts(")\n");

	sched_ready_proc(&G_SCHED, pid);
	sched(&G_SCHED, sched_switch_irq);
	uart_puts("SCHED after\n");
	dump_sched(&G_SCHED, G_VERB);
}

static struct mpsc_ring *g_ipc_ring = (void*)TOJRT_RING_ADDR;
void ipc_sched(ctx_t *ctx)
{
	jrt_sched_req_t sr;
	int budget;
//	uart_puts("periodic call\n");
	for (budget = 0; budget < 3; budget++) {
		if (mpsc_pop(g_ipc_ring, sr.b) != 0)
			break;
		schedule_req(sr.pc, sr.prog_size, sr.mem_req);
	}
	//uart_puts("periodic call\n");
}

void timer_fn(ctx_t *)
{
	proc_t *p;
	u64 dl;
	u64 now;
	uart_puts("TIMER (");
	uart_putu64(time_now_ticks());
	uart_puts(")\n");
	dump_sched(&G_SCHED, G_VERB);

	if (!sched_has_waiting(&G_SCHED)) {
		timer_cancel();
		goto ret;
	}
	do {
		heap_pop(&G_SCHED.waiting, &dl, (void**)&p);
		if (!p)
			goto ret;
		now = time_now_ticks();

		if (dl > now) {
			sched_wait_proc(&G_SCHED, p->pid);
			//heap_push(&G_SCHED.waiting, dl, p);
			goto end;
		}
		uart_puts("timer dl: ");
		uart_putu64(dl);
		uart_puts("\n");
		sched_ready_proc(&G_SCHED, p->pid);
		sched(&G_SCHED, sched_switch_irq);
	} while (sched_has_waiting(&G_SCHED));
	timer_cancel();
	goto ret;
end:
	timer_schedule_at_ticks(dl);
ret:
	uart_puts("TIMER after\n");
	dump_sched(&G_SCHED, G_VERB);
}
