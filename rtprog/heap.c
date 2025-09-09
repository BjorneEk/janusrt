
#include "heap.h"
#include "string.h"

static inline size_t parent(size_t i) { return (i - 1) / HEAP_D; }
static inline size_t child (size_t i, size_t k) { return HEAP_D * i + 1 + k; }

static inline void swap_nodes(minheap_t *h, size_t i, size_t j)
{
	heap_node_t *ti;
	heap_node_t *tj;

	ti = &h->a[i];
	tj = &h->a[j];

	memcpy(&h->a[i], tj, sizeof(heap_node_t));
	tj->idx = i;
	memcpy(&h->a[j], ti, sizeof(heap_node_t));
	ti->idx = j;
}

static inline void sift_up(minheap_t *h, size_t i)
{
	size_t p;
	while (i > 0) {
		p = parent(i);
		if (h->a[p].key <= h->a[i].key)
			break;
		swap_nodes(h, p, i);
		i = p;
	}
}

static inline void sift_down(minheap_t *h, size_t i)
{
	size_t	first,
		best,
		last,
		c;
	for (;;) {
		best = i;
		first = child(i, 0);
		if (first >= h->len)
			break;

		// find min among up to D children
		last = first + HEAP_D;
		if (last > h->len)
			last = h->len;
		for (c = first; c < last; ++c)
			if (h->a[c].key < h->a[best].key)
				best = c;

		if (best == i)
			break;
		swap_nodes(h, i, best);
		i = best;
	}
}

// invokes iterf(last, key, val, arg); for each element,
void heap_iter(minheap_t *h, void *arg, void (*iterf)(bool,u64,void*,void*))
{
	size_t i;

	for (i = 0; i < h->len; ++i)
		iterf(i == h->len - 1, h->a[i].key, h->a[i].val);
}
// Insert node with preset key/val; returns 0 on success, -1 if full.
int heap_push(minheap_t *h, u64 key, void *data)
{
	size_t i;

	if (heap_full(h))
		return -1;
	i = h->len++;
	h->a[i].key = key;
	h->a[i].val = data;
	h->a[i].idx = i;
	sift_up(h, i);
	return 0;
}

// Pop min; returns pointer or NULL if empty.
static inline heap_node_t *heap_pop_node(minheap_t *h)
{
	heap_node_t *min;

	if (heap_empty(h))
		return NULL;

	min = &h->a[0];
	min->idx = SIZE_MAX;

	if (--h->len) {
		h->a[0] = h->a[h->len];
		h->a[0].idx = 0;
		sift_down(h, 0);
	}
	return min;
}
void heap_peek(minheap_t *h, u64 *key, void **data)
{
	if (heap_empty(h)) {
		*data = NULL;
		return;
	}
	*data = h->a[0].val;
	*key = h->a[0].key;
}
void heap_pop(minheap_t *h, u64 *key, void **data)
{
	heap_node_t *n;

	*data = NULL;

	n = heap_pop_node(h);

	if (n) {
		*key = n->key;
		*data = n->val;
	}
}

// Peek min without removing
static inline heap_node_t *heap_min_node(const minheap_t *h)
{
	return heap_empty(h) ? NULL : &h->a[0];
}

// Decrease key (new_key must be <= current key)
static inline void heap_decrease_key(minheap_t *h, heap_node_t *n, uint64_t new_key)
{
	size_t i;

	i = n->idx;
	if (i == SIZE_MAX)
		return; // not in heap
	if (new_key >= n->key)
		return;
	n->key = new_key;
	sift_up(h, i);
}

// If you need increase-key:
static inline void heap_increase_key(minheap_t *h, heap_node_t *n, uint64_t new_key)
{
	size_t i;

	i = n->idx;
	if (i == SIZE_MAX)
		return;
	if (new_key <= n->key)
		return;
	n->key = new_key;
	sift_down(h, i);
}

