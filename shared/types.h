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

	typedef uintptr_t	uptr;
	typedef intptr_t	sptr;
	typedef unsigned	__int128 u128;
	typedef int64_t		ssize_t;
#endif

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif
/* ---------- Alignment + packing macros ---------- */

/* Align a type or object to N bytes */
#if defined(__GNUC__) || defined(__clang__)
# define JRT_ALIGNED(n)   __attribute__((aligned(n)))
#else
# error "Need alignment macro for this compiler"
#endif

/* Pack a struct with no padding */
#if defined(__GNUC__) || defined(__clang__)
# define JRT_PACKED       __attribute__((packed))
#else
# error "Need packed macro for this compiler"
#endif

/* Compile-time static assertion */
#if __STDC_VERSION__ >= 201112L
# define JRT_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#elif defined(__GNUC__)
# define JRT_STATIC_ASSERT(cond, msg) typedef char static_assertion_##msg[(cond)?1:-1] __attribute__((unused))
#else
# define JRT_STATIC_ASSERT(cond, msg) typedef char static_assertion_##msg[(cond)?1:-1]
#endif

/* ---------- Cacheline size (arch-dependent) ---------- */
#ifndef JRT_CACHELINE
# define JRT_CACHELINE 64   /* safe default for arm64, x86_64 */
#endif

/* ---------- Likely/unlikely branch prediction hints ---------- */
#if defined(__GNUC__) || defined(__clang__)
# define JRT_LIKELY(x)    __builtin_expect(!!(x), 1)
# define JRT_UNLIKELY(x)  __builtin_expect(!!(x), 0)
#else
# define JRT_LIKELY(x)   (x)
# define JRT_UNLIKELY(x) (x)
#endif
#endif
