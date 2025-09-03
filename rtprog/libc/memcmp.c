/**
 * Author: Gustaf Franzen <gustaffranzen@icloud.com>
 */

#include "string.h"

int memcmp(const void* lhs, const void* rhs, size_t count)
{
	const unsigned char *l;
	const unsigned char *r;
	const u64 *ll;
	const u64 *rr;
	int diff;
	s64 ldiff;
	l = lhs;
	r = rhs;

	if (count == 0 || l == r)
		return 0;

	while (count && ((uintptr_t)r & 7)) {
		diff = *l++ - *r++;
		if (diff)
			return diff;
		--count;
	}

	ll = (const u64*)l;
	rr = (const u64*)r;
	while (count >= 8) {
		ldiff = *ll++ - *rr++;
		if (ldiff)
			return ldiff < 0 ? -1 : 1;
		count -= 8;
	}

	l = (const unsigned char *)ll;
	r = (const unsigned char *)rr;
	while (count--) {
		diff = *l++ - *r++;
		if (diff)
			return diff;
	}

	return 0;
}
