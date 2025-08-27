
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _KERROR_H_
#define _KERROR_H_

#include "types.h"

typedef enum jrt_err {
	JRT_ENOMEM,
	JRT_EINVAL,
	JRT_EASSERT,
	JRT_EUNKNOWN
} jrt_err_t;

#define KASSERT(expr)							\
	do {								\
		if (!(expr)) {						\
			jrt_fail_kassert(#expr, __FILE__, __func__, __LINE__);	\
		}							\
	} while (0)

#define KERNEL_PANIC(err) do { jrt_seterrno(err);PANIC_BRK(KERNEL_PANIC); } while (0);

void jrt_fail_kassert(const char *str, const char *file, const char *func, u32 line);

const char *jrt_strerror(void);

int jrt_errno(void);

void jrt_seterrno(int err);

void jrt_print_assert(void);

#endif /* _KERROR_H_ */
