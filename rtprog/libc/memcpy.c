/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */


#include "string.h"


void* memcpy(void *dest, const void *src, size_t count)
{
	const unsigned char *s;
	unsigned char *d;
	const u64 *ss;
	u64 *dd;


	d = dest;
	s = src;

	if (count == 0 || d == s)
		return dest;

	while (count && ((uintptr_t)d & 7)) {
		*d++ = *s++;
		--count;
	}

	dd = (u64*)d;
	ss = (const u64*)s;
	while (count >= 8) {
		*dd++ = *ss++;
		count -= 8;
	}

	d = (unsigned char *)dd;
	s = (const unsigned char *)ss;
	while (count--) {
		*d++ = *s++;
	}

	return dest;
}
