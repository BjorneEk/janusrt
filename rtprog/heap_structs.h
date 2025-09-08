// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _HEAP_STRUCTS_H_
#define _HEAP_STRUCTS_H_

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

#endif


