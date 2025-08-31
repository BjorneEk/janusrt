
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _HEAP_H_
#define _HEAP_H_

#include "types.h"

#ifndef HEAP_D
#define HEAP_D 4     // 4-ary heap is a good default
#endif

typedef struct heap_node {
	uint64_t key;	// sort key (deadline, cost, etc.)
	void *val;	// payload (optional)
	size_t idx;	// position in heap->a[], or SIZE_MAX if not in heap
} heap_node_t;

typedef struct minheap {
	heap_node_t *a;	// array of pointers to nodes
	size_t len;	// current size
	size_t cap;	// capacity
} minheap_t;

static inline void heap_init(minheap_t *h, heap_node_t *storage, size_t cap)
{
	h->a   = storage;
	h->len = 0;
	h->cap = cap;
}

static inline int heap_empty(const minheap_t *h) { return h->len == 0; }
static inline int heap_full (const minheap_t *h) { return h->len == h->cap; }

void heap_pop(minheap_t *h, u64 *key, void **data);
void heap_peek(minheap_t *h, u64 *key, void **data);
int heap_push(minheap_t *h, u64 key, void *data);

#endif
