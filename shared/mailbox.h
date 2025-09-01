/**
 * Author Gustaf Franzen <gustaffranzen@icloud.com>
 */
#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include "types.h"

/* ===== Tunables ===== */
#ifndef TOJRT_ORDER
#define TOJRT_ORDER  12                 /* 2^12 = 4096 slots */
#endif
#define TOJRT_SIZE   (1u << TOJRT_ORDER)
#define TOJRT_MASK   (TOJRT_SIZE - 1)

/* Payload size/alignment (compile-time) */
#ifndef TOJRT_REC_SIZE
#define TOJRT_REC_SIZE   32             /* bytes */
#endif
#ifndef TOJRT_REC_ALIGN
#define TOJRT_REC_ALIGN  16             /* pick 1/2/4/8/16… */
#endif

typedef union JRT_PACKED JRT_ALIGNED(TOJRT_REC_ALIGN) tojrt_rec {
	u8 b[24];
	struct {
		u64 pc;
		u64 prog_size;
		u64 mem_req;
	};
} jrt_sched_req_t;

JRT_STATIC_ASSERT(TOJRT_SIZE && !(TOJRT_SIZE & TOJRT_MASK), "ring size must be power of two");
JRT_STATIC_ASSERT(sizeof(jrt_sched_req_t) == TOJRT_REC_SIZE, "rec size mismatch");
JRT_STATIC_ASSERT((TOJRT_REC_ALIGN & (TOJRT_REC_ALIGN-1)) == 0, "align pow2");


/* ====== Ring structure (shared) ======
 * head: next ticket to claim (producers atomically fetch_add)
 * tail: next slot consumer will read (consumer only)
 * flags[i]: 0=empty, 1=full
 */

struct JRT_ALIGNED(JRT_CACHELINE) mpsc_ring {
	u32 head;
	u32 tail;
	u8  _pad0[JRT_CACHELINE - 8];

	u8               flags[TOJRT_SIZE];          /* 0 empty, 1 full */
	jrt_sched_req_t  data [TOJRT_SIZE];          /* payloads */
};
#endif
