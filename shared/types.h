#ifndef _SHARED_TYPES_H_
#define _SHARED_TYPES_H_

#ifdef __KERNEL__
	#include <linux/types.h>
	#include <linux/stddef.h>  /* NULL, bool, true/false if needed */
#else /* --------------------------- Bare metal ---------------------------- */

	#include <stdint.h>
	#include <stddef.h>
	#include <stdbool.h>

	typedef uint8_t   u8;
	typedef uint16_t  u16;
	typedef uint32_t  u32;
	typedef uint64_t  u64;
	typedef int8_t    s8;
	typedef int16_t   s16;
	typedef int32_t   s32;
	typedef int64_t   s64;

	typedef uintptr_t uptr;
	typedef intptr_t  sptr;
#endif
#endif
