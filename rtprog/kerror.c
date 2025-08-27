
#include "kerror.h"
#include "cpu.h"
#include "uart.h"


int g_errno = 0;
struct {
	const char *str;
	const char *file;
	const char *func;
	u32 line;
} g_assert_info;

void jrt_seterrno(int err)
{
	g_errno = err;
}

int jrt_errno(void)
{
	return g_errno;
}

const char *jrt_strerror(void)
{
	switch (g_errno) {
	case JRT_ENOMEM: return "Out of memory";
	case JRT_EINVAL: return "Invalid argument";
	case JRT_EASSERT: return "Assertion error";
	case JRT_EUNKNOWN:
	default: return "Unknown error";
	}
}

void jrt_fail_kassert(
	const char *str,
	const char *file,
	const char *func,
	u32 line)
{
	g_assert_info.str = str;
	g_assert_info.file = file;
	g_assert_info.func = func;
	g_assert_info.line = line;
	KERNEL_PANIC(JRT_EASSERT);
}

void jrt_print_assert(void)
{

	uart_puts("file: ");
	uart_puts(g_assert_info.file);
	uart_puts(":");
	uart_puts(g_assert_info.func);
	uart_puts(":");
	uart_putu32(g_assert_info.line);
	uart_puts(":\n");
	uart_puts(g_assert_info.str);
	uart_puts("\n");
}

