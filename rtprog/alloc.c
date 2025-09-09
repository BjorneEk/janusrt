

// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#include "alloc.h"
#include "types.h"
#include "string.h"
#include "kerror.h"
#include "uart.h"
// simple linear best fit allocator


typedef struct ablock {
	u32 prev; // distance to previous block
	u32 next; // distance to next block (equivalent to total block size)
	u32 used; // used 1
	u32 next_free; // distance to next free block (end on none)
	u32 prev_free; // distance to previous free block (NULL on none)
} ablock_t;

#define USED_HEADER_SIZE (3 * sizeof(u32))
#define UNUSED_HEADER_SIZE (sizeof(ablock_t))
#define BLOCK_PTR(_b) ((void*)((uintptr_t)_b + (uintptr_t)USED_HEADER_SIZE))
#define PTR_BLOCK(_p) ((ablock_t*)((uintptr_t)_p - (uintptr_t)USED_HEADER_SIZE))

#define MIN_SPLIT_THRESHOLD (2 * UNUSED_HEADER_SIZE)


void alloc_init(alloc_t *a, void *base, size_t size)
{
	a->base = base;
	a->free = base;
	a->end = (ablock_t*)((uintptr_t)base + size);
	a->free->prev = 0;
	a->free->next = size;
	a->free->used = 0;
	a->free->next_free = size;
	a->free->prev_free = 0;
}

static ablock_t *nfree(ablock_t *b)
{
	return (ablock_t*)((uintptr_t)b + (uintptr_t)b->next_free);
}

static ablock_t *pfree(ablock_t *b)
{
	return (ablock_t*)((uintptr_t)b - (uintptr_t)b->prev_free);
}

static ablock_t *next(ablock_t *b)
{
	return (ablock_t*)((uintptr_t)b + (uintptr_t)b->next);
}

static ablock_t *prev(ablock_t *b)
{
	if (b->prev == 0)
		return NULL;

	return (ablock_t*)((uintptr_t)b - (uintptr_t)b->prev);
}


static void split(alloc_t *a, ablock_t *b, u32 size)
{
	ablock_t *s, *n;

	s = (ablock_t*)((uintptr_t)b + (uintptr_t)size);
	s->next = b->next - size;
	b->next = size;
	s->prev = size;

	s->next_free = b->next_free - size;
	b->next_free = b->next;
	s->prev_free = s->prev;
	s->used = 0;

	n = next(s);
	if (n != a->end)
		n->prev -= size;

	n = nfree(s);
	if (n != a->end)
		n->prev_free -= size;
}

static void use_block(alloc_t *a, ablock_t *b)
{
	ablock_t *nf,
		 *pf;

	if (b == a->free) {
		a->free = nfree(b);
		a->free->prev_free = 0;
	} else {
		pf = pfree(b);
		pf->next_free += b->next_free;
		nf = nfree(b);
		if (nf != a->end)
			nf->prev_free += b->prev_free;
	}
	b->used = 1;
}

void *alloc(alloc_t *a, size_t len)
{
	size_t xlen;
	ablock_t *best,
		 *curr;

	xlen = len + USED_HEADER_SIZE;
	curr = a->free;
	best = NULL;

	// find best fit
	while (curr != a->end) {
		if (curr->next >= xlen && (!best || best->next > curr->next))
			best = curr;
		curr = nfree(curr);
	}

	if (!best)
		return NULL;

	// split block to size
	if ((best->next - xlen) >= MIN_SPLIT_THRESHOLD)
		split(a, best, xlen);

	// cut out block from free list
	use_block(a, best);

	return BLOCK_PTR(best);
}
static u64 get_align(void *p, u64 align)
{
	return ((uintptr_t)p & (align - 1));
}

void *aligned_alloc(alloc_t *a, size_t len, u64 align)
{
	size_t xlen;
	ablock_t *best,
		 *curr;
	u64 ca;
	xlen = len + USED_HEADER_SIZE;
	curr = a->free;
	best = NULL;

	// find best fit
	while (curr != a->end) {
		if (curr->next >= xlen) {
			ca = get_align(BLOCK_PTR(curr), align);
			// already aligned
			if ((ca == 0) && (!best || best->next > curr->next) ) {
				best = curr;
			// can fit an aligned
			} else if (curr->next >= ((align - ca) + xlen)) {
				best = curr;
			}
		}
		curr = nfree(curr);
	}

	if (!best)
		return NULL;

	ca = get_align(BLOCK_PTR(best), align);
	// need to split of unaligned begining
	if (ca != 0) {
		split(a, best, align - ca);
		best = next(best);
	}
	// split block to size
	if ((best->next - xlen) >= MIN_SPLIT_THRESHOLD)
		split(a, best, xlen);

	// cut out block from free list
	use_block(a, best);

	return BLOCK_PTR(best);
}
void free(alloc_t *a, void *ptr)
{
	ablock_t *b,
		 *p,
		 *n,
		 *nn;

	KASSERT(ptr < (void*)a->end);
	KASSERT(ptr > (void*)a->base);
	b = PTR_BLOCK(ptr);
	//uart_puts("\n\nused: ");
	//uart_putu32(b->used);
	//uart_puts("\n\n");
	KASSERT(b->used == 1);

	b->prev_free = 0;
	b->used = 0;

	if (b < a->free)
		a->free = b;

	// find next free
	n = next(b);
	while (n != a->end && n->used)
		n = next(n);

	// find prev free (if next exists, prev is nexts prev
	if (n != a->end) {
		p = pfree(n);
	} else {
		p = prev(b);
		while (p != NULL && p->used)
			p = prev(p);
	}

	// relink next
	b->next_free = (u32)((uintptr_t)n - (uintptr_t)b);
	if (n != a->end)
		n->prev_free = b->next_free;

	//relink prev
	if (p) {
		b->prev_free = (u32)((uintptr_t)b - (uintptr_t)p);
		p->next_free = b->prev_free;
	}
	// coalesce
	do {
		if (n != a->end && n == next(b)) {
			nn = nfree(n);
			if (nn != a->end)
				nn->prev_free += b->next_free;

			nn = next(n);
			if (nn != a->end)
				nn->prev += b->next;

			b->next_free += n->next_free;
			b->next += n->next;
		}
		n = b;
		b = p;
		p = NULL;
	} while (b);
}

void *realloc(alloc_t *a, void *p, size_t size)
{
	ablock_t *b,
		 *n,
		 *nn;
	void *np;
	size_t blen, nlen;

	KASSERT(p < (void*)a->end);
	KASSERT(p > (void*)a->base);
	b = PTR_BLOCK(p);
	KASSERT(b->used == 1);


	blen = b->next - USED_HEADER_SIZE;
	n = next(b);

	if (n == a->end)
		goto new;
	if (n->used)
		goto new;
	if ((blen + n->next) < size)
		goto new;

	// in place case
	nlen = size - blen;
	if ((n->next - nlen) >= MIN_SPLIT_THRESHOLD)
		split(a, n, nlen);

	use_block(a, n);

	// resize
	nn = next(n);
	if (nn != a->end)
		nn->prev += b->next;
	b->next += n->next;
	return p;
new:
	np = alloc(a, size);
	if (!np)
		return NULL;
	memcpy(np, p, blen);
	free(a, p);
	return np;
}




