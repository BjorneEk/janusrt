
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _SYSCALL_H_
#define _SYSCALL_H_
#include "uart.h"
#include "stdarg.h"

typedef enum syscall {
	SYSCALL_WAIT_UNTIL,
	SYSCALL_EXIT
} syscall_t;


void take_syscall(u16 imm);
void syscall(syscall_t call, ...);
#endif
