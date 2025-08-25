
/**
 * Author Gustaf Franzen <gustaffranzen@icloud.com>
 */
#ifndef _RTCORE_H_
#define _RTCORE_H_

#include "types.h"

//#define RTCORE_MAGIC (0xB0)
//#define RTCORE_BOOT _IOW(RTCORE_MAGIC, 0x01, struct rtcore_boot)

//struct rtcore_start_args {
//	u32 core_id;
//	u32 entrypoint;
//};

#define DEVICE_NAME "rtcore"

typedef struct rtcore_start_args {
	uint64_t entry_user;
	uint64_t core_id;
} start_cpu_args_t;

typedef struct rtcore_sched_args {
	uint64_t phys_addr;
} sched_prog_args_t;



#define RTCORE_IOCTL_START_CPU	_IOW('r', 1, start_cpu_args_t)
#define RTCORE_IOCTL_SCHED_PROG	_IOW('r', 2, sched_prog_args_t)

#define CACHELINE	64
#define RING_ORDER	12
#define RING_SIZE	(1u << RING_ORDER)	// 4096 entries
#define RING_MASK	(RING_SIZE - 1)

struct __attribute__((aligned(CACHELINE))) spsc_ring {
	// producer writes head, consumer writes tail
	_Atomic u32	head;	// next slot to write (prod)
	_Atomic u32	tail;	// next slot to read (cons)
	u32		_pad0[14];

	// slot-valid flags to avoid extra fences per slot (optional)
	_Atomic u8		flags[RING_SIZE]; // 0: empty, 1: full

	// fixed-size messages to keep it fast; tune to your needs
	struct {
		u16 type;
		u16 len;
		u8  payload[56];	// 64-byte slots
	} __attribute__((packed, aligned(64))) data[RING_SIZE];
};

typedef struct __attribute__((aligned(CACHELINE))) jrt_kernel_mailbox {
	u32	version;
	u32	flags;
	u64	cntfrq_hz;		// fill by Linux for RT's info
	u64	reserved0;

	// Linux -> RT ring
	struct spsc_ring	l2r;

	// RT -> Linux ring
	struct spsc_ring	r2l;

	// doorbell counters (optional, for debugging)
	_Atomic u64	l2r_dbell;
	_Atomic u64	r2l_dbell;
} jrt_kernel_mailbox_t;

#define PROG_CAP (64)
typedef struct mem_share {
	jrt_kernel_mailbox_t mailbox;

	u64 size;
	u64 cap;

	u64 programs[PROG_CAP];
	u32 nprograms;

	u8 mem[];
} mem_share_t;

#endif
