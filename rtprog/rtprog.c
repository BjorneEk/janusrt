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

sched_t G_SCHED;
alloc_t G_ALLOC;
ctx_t *G_KERNEL_CTX;

void periodic_func(ctx_t *);

static void trampoline(ctx_t *c)
{
	timer_irq_handler(c);
}

void timer_init(void)
{
	irq_register_ppi(EL1_PHYS_TIMER_PPI, trampoline);
	set_timer_func(periodic_func);
	start_periodic_task_freq(1);
}
void sched_spi_init(void)
{
	irq_register_spi(SCHED_SPI, periodic_func);
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
	G_SCHED.pid = 0;

	uart_puts("enter loop\n");
	for (;;) {
		while (heap_empty(&G_SCHED.ready))
			wfe();
		//sched(&G_SCHED, sched_switch_sync);
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

void schedule_req(u64 pc, u64 prog_size, u64 mem_req)
{
	u32 pid;
	void *mem;

	//interrupts_disable_all();
	mem = alloc(&G_ALLOC, mem_req);

	if (!mem)
		KERNEL_PANIC(JRT_ENOMEM);

	pid = sched_new_proc(
		&G_SCHED,
		pc,
		prog_size,
		mem,
		mem_req,
		0,
		jrt_loop);
	uart_puts("[SCHED] ");
	uart_putu32(pid);
	uart_puts(": (");
	uart_putu64(pc);
	uart_puts(", ");
	uart_putu64(prog_size);
	uart_puts(", ");
	uart_putu64(mem_req);
	uart_puts(")\n");

	sched_sched_proc(&G_SCHED, pid);
	sched(&G_SCHED, sched_switch_irq);
}

static struct mpsc_ring *g_ipc_ring = (void*)TOJRT_RING_ADDR;
void periodic_func(ctx_t *ctx)
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

