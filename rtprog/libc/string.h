/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */
#ifndef _STRING_H_
#define _STRING_H_

#include "types.h"

void* memcpy(void *dest, const void *src, size_t count);

void *memset(void *dest, int ch, size_t count);

int memcmp(const void* lhs, const void* rhs, size_t count);
#endif

