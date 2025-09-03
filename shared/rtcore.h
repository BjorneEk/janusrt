
/**
 * Author Gustaf Franzen <gustaffranzen@icloud.com>
 */
#ifndef _RTCORE_H_
#define _RTCORE_H_

#include "types.h"
#include "mailbox.h"

#define DEVICE_NAME "rtcore"

typedef struct rtcore_start_args {
	uint64_t entry_user;
	uint64_t core_id;
} start_cpu_args_t;

typedef struct rtcore_sched_args {
	uint64_t entry_user;
	uint64_t mem_req;
} sched_prog_args_t;

#define RTCORE_IOCTL_START_CPU	_IOW('r', 1, start_cpu_args_t)
#define RTCORE_IOCTL_SCHED_PROG	_IOW('r', 2, sched_prog_args_t)

#define SCHED_SPI (72)

#endif
