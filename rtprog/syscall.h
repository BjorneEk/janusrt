
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _SYSCALL_H_
#define _SYSCALL_H_
#include "uart.h"

typedef enum syscall {
	SYSCALL_EXIT,
	SYSCALL_YIELD,
	SYSCALL_WAIT_UNTIL
} syscall_t;

void syscall(u16 imm);
#endif
