
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#include "syscall.h"
#include "sched.h"

extern sched_t G_SCHED;

void syscall(u16 imm)
{
	uart_puts("syscall!\n");
}
