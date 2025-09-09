
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _ALLOC_H_
#define _ALLOC_H_

#include "types.h"
// a simple doubly linked best fit allocator for now, dont want to waste time here
struct ablock;

typedef struct allocator {
	void *base;
	struct ablock *free;
	struct ablock *end;
} alloc_t;

void alloc_init(alloc_t *a, void *base, size_t size);

void *alloc(alloc_t *a, size_t len);

void free(alloc_t *a, void *p);

void *realloc(alloc_t *a, void *p, size_t len);

void *aligned_alloc(alloc_t *a, size_t len, u64 align);

void dump_alloc(alloc_t *a);
#endif
