
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _CPU_H_
#define _CPU_H_


enum panic_reason {
	KERNEL_PANIC,
	USER_PANIC
};

typedef void (*sync_recover_t)(enum panic_reason);
static inline const char *panic_str(enum panic_reason r)
{
	switch (r) {
		case KERNEL_PANIC: return "Kernel panic";
		case USER_PANIC: return "User panic";
		default: return "Unknown panic";
	}
}

#define PANIC_BRK(code) do { asm volatile("brk %0" :: "I"(code)); } while (0)
//#define KERNEL_PANIC() PANIC_BRK(KERNEL_PANIC)
//#define USER_PANIC() USER_BRK(KERNEL_PANIC)

static inline void wfe(void)
{
	asm volatile("wfe" ::: "memory");
}

static inline void halt(void)
{
	for (;;)
		wfe();
}

// in sync.c
void sync_set_recover_handler(sync_recover_t recover);
#endif
