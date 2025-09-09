/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */
#ifndef _STRING_H_
#define _STRING_H_

#include "types.h"
#include "arg.h"
void* memcpy(void *dest, const void *src, size_t count);

void *memset(void *dest, int ch, size_t count);

int memcmp(const void* lhs, const void* rhs, size_t count);

int vsnprintf(char *dst, size_t n, const char *fmt, va_list ap);

int snprintf(char *dst, size_t n, const char *fmt, ...);

#endif

