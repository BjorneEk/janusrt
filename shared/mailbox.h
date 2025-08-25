/**
 * Author Gustaf Franzen <gustaffranzen@icloud.com>
 */
#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include "types.h"

/* ====== Tunables ====== */
#ifndef TOJRT_ORDER
#define TOJRT_ORDER  12                  /* 2^12 = 4096 slots */
#endif
#define TOJRT_SIZE   (1u << TOJRT_ORDER)
#define TOJRT_MASK   (TOJRT_SIZE - 1)

struct JRT_PACKED tojrt_rec {
	u8 b[16];
};
JRT_STATIC_ASSERT(sizeof(struct tojrt_rec) == 16, "rec16_size");


/* ====== Ring structure (shared) ======
 * head: next ticket to claim (producers atomically fetch_add)
 * tail: next slot consumer will read (consumer only)
 * flags[i]: 0=empty, 1=full
 */
struct JRT_ALIGNED(JRT_CACHELINE) mpsc_ring {
	u32 head;                     /* producer ticket (wraps naturally) */
	u32 tail;                     /* consumer cursor */
	u8 _pad0[JRT_CACHELINE - 8]; /* avoid false sharing */

	u8 flags[TOJRT_SIZE];       /* one byte per slot: 0 empty, 1 full */
	struct tojrt_rec data[TOJRT_SIZE];
};

#endif
