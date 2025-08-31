/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */

#include "string.h"

void *memset(void *dest, int ch, size_t count)
{
	unsigned char c;
	u64 lc;
	unsigned char *d;
	u64 *ld;

	d = dest;
	c = ch;

	if (count == 0)
		return dest;

	while (count && ((uintptr_t)d & 7)) {
		*d++ = c;
		--count;
	}

	if (count >= 8) {
		lc = (u64)c;
		lc |= lc << 8;
		lc |= lc << 16;
		lc |= lc << 32;

		ld = (u64*)d;
		while (count >= 8) {
			*ld++ = lc;
			count -= 8;
		}
		d = (unsigned char *)ld;
	}


	while (count--)
		*d++ = c;

	return dest;
}
