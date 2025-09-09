


// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#include "types.h"
#include "string.h"
#include "arg.h"

// simple linear best fit allocator

// ===== Utilities =====

static inline int is_digit(char c) { return (unsigned)(c - '0') <= 9u; }

typedef struct {
	char	*buf;
	size_t	cap;	// total capacity
	size_t	len;	// produced length (<= cap if not truncated)
} out_t;

static inline void out_init(out_t *o, char *dst, size_t n)
{
	o->buf = dst;
	o->cap = n ? n - 1 : 0;	// keep space for NUL
	o->len = 0;
}

static inline void out_putc(out_t *o, char c)
{
	if (o->len < o->cap)
		o->buf[o->len] = c;
	o->len++;
}

static inline void out_write_n(out_t *o, const char *s, size_t n)
{
	size_t i;
	// fast copy without libc
	for (i = 0; i < n; i++)
		out_putc(o, s[i]);
}

static inline void out_terminate(out_t *o)
{
	size_t at;
	if (o->cap > 0) {
		at = (o->len < o->cap) ? o->len : o->cap;
		o->buf[at] = '\0';
	} else if (o->buf) {
		// n==0 => nothing to write; nothing to terminate
	}
}

// compute string length without libc
static size_t cstr_len(const char *s)
{
	const char *p;

	p = s;
	while (*p)
		p++;
	return (size_t)(p - s);
}

// ===== Parse result =====

typedef struct {
	int	param_idx;	// (not used; no n$ support)
	unsigned	flags;	// bitset: 1='-', 2='+', 4='0', 8='#', 16=' '
	int		width;	// -1 = not set, or value / from '*'
	int		prec;	// -1 = not set, else >= 0
	enum { L_none, L_hh, L_h, L_l, L_ll, L_j, L_z, L_t, L_L } len;
	char	type;
} fmt_t;

enum {
	F_LEFT = 1u << 0,
	F_SIGN = 1u << 1,
	F_ZERO = 1u << 2,
	F_ALT  = 1u << 3,
	F_SP   = 1u << 4
};

// ===== Integer conversion =====

static size_t utoa_rev(char *tmp, uint64_t v, unsigned base, int upper)
{
	// returns length; digits are written in reverse order into tmp
	static const char digs_l[] = "0123456789abcdef";
	static const char digs_u[] = "0123456789ABCDEF";
	const char *digs;
	size_t n;
	unsigned d;

	digs = upper ? digs_u : digs_l;

	n = 0;
	if (v == 0) {
		tmp[n++] = '0';
		return n;
	}
	while (v) {
		d = (unsigned)(v % base);
		v /= base;
		tmp[n++] = digs[d];
	}
	return n;
}

static void pad(out_t *o, char c, int count)
{
	int i;
	for (i = 0; i < count; i++)
		out_putc(o, c);
}

static void emit_int(out_t *o, const fmt_t *f, uint64_t uval, int neg, unsigned base, int upper)
{
	// precision overrides zero-pad, per printf rules
	char tmp[64];	// enough for uint64 in base 2..16 + extras
	char pre[2];
	size_t i, ndig, pre_len;
	char sign_ch;
	int zeros, minw, total_len, pad_len;

	ndig = utoa_rev(tmp, uval, base, upper);

	// precision==0 and value==0 => print nothing
	if (f->prec == 0 && uval == 0) ndig = 0;

	// sign / prefix
	sign_ch = 0;
	if (neg)
		sign_ch = '-';
	else if (f->flags & F_SIGN)
		sign_ch = '+';
	else if (f->flags & F_SP)
		sign_ch = ' ';

	pre_len = 0;
	if ((f->flags & F_ALT) && uval != 0) {
		if (base == 8) { pre[pre_len++] = '0'; }
		else if (base == 16) {
			pre[pre_len++] = '0';
			pre[pre_len++] = upper ? 'X' : 'x';
		}
	}

	zeros = 0;
	if (f->prec >= 0) {
		if ((int)ndig < f->prec)
			zeros = f->prec - (int)ndig;
	} else if ((f->flags & F_ZERO) && !(f->flags & F_LEFT) && f->width > 0) {
		minw = (int)ndig + zeros + (int)pre_len + (sign_ch ? 1 : 0);
		if (minw < f->width)
			zeros = f->width - minw;
	}

	total_len = (int)ndig + zeros + (int)pre_len + (sign_ch ? 1 : 0);
	pad_len = (f->width > total_len) ? (f->width - total_len) : 0;

	if (!(f->flags & F_LEFT))
		pad(o, ' ', pad_len);
	if (sign_ch)
		out_putc(o, sign_ch);
	for (i = 0; i < pre_len; i++)
		out_putc(o, pre[i]);
	pad(o, '0', zeros);
	// digits were reversed
	for (i = 0; i < ndig; i++)
		out_putc(o, tmp[ndig - 1 - i]);
	if (f->flags & F_LEFT)
		pad(o, ' ', pad_len);
}

// ===== Floating conversion (simple, rounded, finite/inf/nan) =====
//
// We avoid libc. This is a compact dtoa good enough for %f/%e/%g.
// Rounds to nearest, handles precision (default 6), sign, inf/nan.
// For %g, trims trailing zeros and switches e/f per rules.

static int fp_is_nan(double x)
{
	union { double d; uint64_t u; } v = { x };
	return ((v.u & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL) &&
		(v.u & 0x000fffffffffffffULL);
}

static int fp_is_inf(double x)
{
	union { double d; uint64_t u; } v = { x };
	return ((v.u & 0x7fffffffffffffffULL) == 0x7ff0000000000000ULL);
}

static double fp_abs(double x)
{
	return x < 0.0 ? -x : x;
}

// pow10 table for quick scaling
static const double fp_p10[] = {
	1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10,1e11,1e12,1e13,1e14,1e15
};

static double fp_pow10(int n)
{
	double r;
	int k;
	// limited fast pow10 for |n| <= ~308 using loops
	if (n == 0)
		return 1.0;

	r = 1.0;
	k = (n < 0) ? -n : n;

	while (k >= 15) {
		r *= 1e15;
		k -= 15;
	}

	if (k)
		r *= fp_p10[k];

	return (n < 0) ? (1.0 / r) : r;
}

static void emit_inf_nan(out_t *o, const fmt_t *f, int neg, int upper, int is_nan)
{
	const char *s;
	char sign_ch;
	int n, pad_len;

	s = is_nan ? (upper ? "NAN" : "nan") : (upper ? "INF" : "inf");
	sign_ch = 0;

	if (!is_nan) {
		if (neg)
			sign_ch = '-';
		else if (f->flags & F_SIGN)
			sign_ch = '+';
		else if (f->flags & F_SP)
			sign_ch = ' ';
	} else {
		if (f->flags & F_SIGN)
			sign_ch = '+';
		else if (f->flags & F_SP)
			sign_ch = ' ';
	}

	n = (int)cstr_len(s) + (sign_ch ? 1 : 0);
	pad_len = (f->width > n) ? (f->width - n) : 0;
	if (!(f->flags & F_LEFT))
		pad(o, ' ', pad_len);
	if (sign_ch)
		out_putc(o, sign_ch);
	out_write_n(o, s, (size_t)(f->type < 'a' ? 3 : 3)); // already upper/lower chosen
	if (f->flags & F_LEFT)
		pad(o, ' ', pad_len);
}

static void emit_float_f(out_t *o, const fmt_t *f, double x, int upper)
{
	char itmp[32], ftmp[24];
	double r, y;
	uint64_t yi, int_part, frac_part;
	size_t in, fn, i;
	int neg, prec, dot, sign_ch, total, pad_len;

	neg = (x < 0.0);
	if (fp_is_nan(x)) {
		emit_inf_nan(o, f, 0, upper, 1);
		return;
	}
	if (fp_is_inf(x)) {
		emit_inf_nan(o, f, neg, upper, 0);
		return;
	}
	x = fp_abs(x);

	prec = (f->prec >= 0) ? f->prec : 6;
	if (prec > 18)
		prec = 18; // keep it bounded

	// Round by adding 0.5 ulp of the last digit
	r = fp_pow10(prec);
	y = x * r;
	yi = (uint64_t)(y + 0.5);
	int_part = yi / (uint64_t)r;
	frac_part = yi - int_part * (uint64_t)r;

	// convert integer part
	in = utoa_rev(itmp, int_part, 10, 0);

	// convert fraction (fixed width with leading zeros)
	fn = 0;
	for (i = 0; i < prec; i++) {
		ftmp[prec - 1 - i] = (char)('0' + (frac_part % 10));
		frac_part /= 10;
	}
	fn = (size_t)prec;

	// optional trimming if precision==0
	dot = (prec > 0);

	// compute length for padding
	sign_ch = 0;
	if (neg)
		sign_ch = '-';
	else if (f->flags & F_SIGN)
		sign_ch = '+';
	else if (f->flags & F_SP)
		sign_ch = ' ';

	total = (sign_ch ? 1 : 0) + (int)in + (dot ? 1 : 0) + (int)fn;
	pad_len = (f->width > total) ? (f->width - total) : 0;

	if (!(f->flags & F_LEFT))
		pad(o, (f->flags & F_ZERO) ? '0' : ' ', pad_len);

	if (sign_ch)
		out_putc(o, (char)sign_ch);

	for (i = 0; i < in; i++)
		out_putc(o, itmp[in - 1 - i]);

	if (dot) {
		out_putc(o, '.');
		out_write_n(o, ftmp, fn);
	}

	if (f->flags & F_LEFT)
		pad(o, ' ', pad_len);
}

static void emit_float_e(out_t *o, const fmt_t *f, double x, int upper)
{
	double r, y;
	uint64_t yi, ip, fp;
	size_t mn, en;
	int neg, prec, exp10, aexp, d2, total, pad_len, i;
	char sign_ch, ebuf[8], mt[32];


	neg = (x < 0.0);
	if (fp_is_nan(x)) {
		emit_inf_nan(o, f, 0, upper, 1);
		return;
	}
	if (fp_is_inf(x)) {
		emit_inf_nan(o, f, neg, upper, 0);
		return;
	}
	x = fp_abs(x);

	prec = (f->prec >= 0) ? f->prec : 6;
	if (prec > 18)
		prec = 18;

	exp10 = 0;
	if (x > 0.0) {
		// Normalize: 1.0 <= m < 10.0
		while (x >= 10.0) {
			x *= 0.1;
			exp10++;
		}
		while (x < 1.0)  {
			x *= 10.0;
			exp10--;
		}
	}
	// Round mantissa to 'prec' digits after decimal
	r = fp_pow10(prec);
	y = x * r;
	yi = (uint64_t)(y + 0.5);
	if (yi >= (uint64_t)fp_pow10(prec + 1)) { // carry overflow, e.g., 9.999.. -> 10.000..
		yi = yi / 10;
		exp10++;
	}
	ip = yi / (uint64_t)r;
	fp = yi - ip * (uint64_t)r;

	// Build mantissa: one digit '.' rest
	mn = 0;
	mt[mn++] = (char)('0' + (char)ip);
	if (prec > 0 || (f->flags & F_ALT)) {
		mt[mn++] = '.';
		for (i = prec - 1; i >= 0; i--) {
			mt[mn + i] = (char)('0' + (fp % 10));
			fp /= 10;
		}
		mn += (size_t)prec;
	}

	// exponent part: e±dd (at least two digits)
	en = 0;
	ebuf[en++] = upper ? 'E' : 'e';
	ebuf[en++] = (exp10 < 0) ? '-' : '+';
	aexp = (exp10 < 0) ? -exp10 : exp10;
	d2 = aexp % 10; int d1 = (aexp / 10) % 10; int d0 = (aexp / 100) % 10;
	if (aexp >= 100) {
		ebuf[en++] = (char)('0' + d0);
		ebuf[en++] = (char)('0' + d1);
		ebuf[en++] = (char)('0' + d2);
	} else {
		ebuf[en++] = (char)('0' + d1);
		ebuf[en++] = (char)('0' + d2);
	}

	// sign and padding
	sign_ch = 0;
	if (neg)
		sign_ch = '-';
	else if (f->flags & F_SIGN)
		sign_ch = '+';
	else if (f->flags & F_SP)
		sign_ch = ' ';

	total = (sign_ch ? 1 : 0) + (int)mn + (int)en;
	pad_len = (f->width > total) ? (f->width - total) : 0;

	if (!(f->flags & F_LEFT))
		pad(o, (f->flags & F_ZERO) ? '0' : ' ', pad_len);
	if (sign_ch)
		out_putc(o, sign_ch);
	out_write_n(o, mt, mn);
	out_write_n(o, ebuf, en);
	if (f->flags & F_LEFT)
		pad(o, ' ', pad_len);
}

static void trim_trailing_zeros(char *buf, size_t *n)
{
	size_t i, j, k, dot;
	// remove trailing zeros in fractional part for %g (but keep one digit before cutting all)
	i = *n;
	dot = (size_t)-1;
	for (k = 0; k < i; k++)
		if (buf[k] == '.') {
			dot = k;
			break;
		}
	if (dot == (size_t)-1)
		return;
	j = i;
	while (j > dot + 1 && buf[j - 1] == '0')
		j--;
	if (j > dot + 1 && buf[j - 1] == '.')
		j--; // strip dot if nothing follows and alt not set
	*n = j;
}

static void emit_float_g(out_t *o, const fmt_t *f, double x, int upper)
{
	fmt_t ff;
	out_t tmp;
	double ax, y;
	size_t n;
	int prec, neg, exp10, use_e, before, frac, pad_len;
	char buf[128];

	// choose e or f presentation
	prec = (f->prec >= 0) ? f->prec : 6;
	if (prec == 0)
		prec = 1; // printf quirk
	// Get magnitude
	neg = (x < 0.0);
	if (fp_is_nan(x)) {
		emit_inf_nan(o, f, 0, upper, 1);
		return;
	}
	if (fp_is_inf(x)) {
		emit_inf_nan(o, f, neg, upper, 0);
		return;
	}
	ax = fp_abs(x);

	// quick decision using exponent estimate
	exp10 = 0;
	if (ax > 0.0) {
		y = ax;
		while (y >= 10.0) {
			y *= 0.1;
			exp10++;
		}
		while (y < 1.0)  {
			y *= 10.0;
			exp10--;
		}
	}
	// Use e if exp < -4 or exp >= prec, else f
	use_e = (exp10 < -4) || (exp10 >= prec);

	// We'll produce into a small stack buffer then pad
	n = 0;
	out_init(&tmp, buf, sizeof(buf));
	tmp.cap = sizeof(buf) - 1;

	ff = *f; // local copy
	if (use_e) {
		// in %g, precision means significant digits; our %e uses frac-digits
		ff.prec = prec - 1;
		emit_float_e(&tmp, &ff, neg ? -ax : ax, upper);
	} else {
		// digits after decimal = prec - (digits before decimal)
		// Estimate digits before decimal as exp10+1 if >=0 else 1
		before = (exp10 >= 0) ? (exp10 + 1) : 1;
		frac = prec - before;
		if (frac < 0)
			frac = 0;
		ff.prec = frac;
		emit_float_f(&tmp, &ff, neg ? -ax : ax, upper);
	}
	out_terminate(&tmp);
	// If '#' not set, trim trailing zeros and maybe dot
	if (!(f->flags & F_ALT)) {
		n = cstr_len(buf);
		trim_trailing_zeros(buf, &n);
	} else {
		n = cstr_len(buf);
	}
	// Apply width/sign/zero was already handled inside, but re-pad if needed
	pad_len = 0;
	if ((int)n < f->width)
		pad_len = f->width - (int)n;
	if (!(f->flags & F_LEFT))
		pad(o, ' ', pad_len);
	out_write_n(o, buf, n);
	if (f->flags & F_LEFT)
		pad(o, ' ', pad_len);
}

// ===== Parser =====

static const char *parse_flags(const char *p, unsigned *flags)
{
	for (;;) {
		switch (*p) {
		case '-': *flags |= F_LEFT; p++; continue;
		case '+': *flags |= F_SIGN; p++; continue;
		case '0': *flags |= F_ZERO; p++; continue;
		case '#': *flags |= F_ALT;  p++; continue;
		case ' ': *flags |= F_SP;   p++; continue;
		default: return p;
		}
	}
}

static const char *parse_width_prec(const char *p, int *val, va_list *ap)
{
	int v, any;

	if (*p == '*') {
		v = va_arg(*ap, int);
		if (v < 0) {
			*val = -v;
			return p + 1;
		} // caller handles '-' by flag later
		*val = v;
		return p + 1;
	}
	v = 0;
	any = 0;
	while (is_digit(*p)) {
		any = 1;
		v = v * 10 + (*p - '0');
		p++;
	}
	*val = any ? v : -1;
	return p;
}

static const char *parse_len(const char *p, int *len_code)
{
	if (p[0] == 'h' && p[1] == 'h')	{ *len_code = L_hh; return p + 2; }
	if (p[0] == 'h')		{ *len_code = L_h;  return p + 1; }
	if (p[0] == 'l' && p[1] == 'l')	{ *len_code = L_ll; return p + 2; }
	if (p[0] == 'l')		{ *len_code = L_l;  return p + 1; }
	if (p[0] == 'j')		{ *len_code = L_j;  return p + 1; }
	if (p[0] == 'z')		{ *len_code = L_z;  return p + 1; }
	if (p[0] == 't')		{ *len_code = L_t;  return p + 1; }
	if (p[0] == 'L')		{ *len_code = L_L;  return p + 1; }
	return p;
}

// fetch integer with length modifier
static uint64_t get_uint_arg(va_list *ap, int len_code)
{
	switch (len_code) {
	case L_hh:	return (unsigned char)va_arg(*ap, unsigned int);
	case L_h:	return (unsigned short)va_arg(*ap, unsigned int);
	case L_l:	return (unsigned long)va_arg(*ap, unsigned long);
	case L_ll:	return (unsigned long long)va_arg(*ap, unsigned long long);
	case L_j:	return (uintmax_t)va_arg(*ap, uintmax_t);
	case L_z:	return (size_t)va_arg(*ap, size_t);
	case L_t:	return (ptrdiff_t)va_arg(*ap, ptrdiff_t);
	default:	return (unsigned int)va_arg(*ap, unsigned int);
	}
}

static int64_t get_int_arg(va_list *ap, int len_code)
{
	switch (len_code) {
	case L_hh: return (signed char)va_arg(*ap, int);
	case L_h:  return (short)va_arg(*ap, int);
	case L_l:  return (long)va_arg(*ap, long);
	case L_ll: return (long long)va_arg(*ap, long long);
	case L_j:  return (intmax_t)va_arg(*ap, intmax_t);
	case L_z:  return (ssize_t)va_arg(*ap, ssize_t);
	case L_t:  return (ptrdiff_t)va_arg(*ap, ptrdiff_t);
	default:   return (int)va_arg(*ap, int);
	}
}

// ===== Core =====

int vsnprintf(char *dst, size_t n, const char *fmt, va_list ap)
{
	out_t out;
	fmt_t f, g;
	size_t sl, written;
	int64_t v;
	uint64_t u;
	uintptr_t pt;
	int any, pad_len;
	char ch;
	const char *p = fmt;

	out_init(&out, dst, n);

	while (*p) {
		if (*p != '%') {
			out_putc(&out, *p++);
			continue;
		}
		p++; // skip '%'

		memset(&f, 0, sizeof(fmt_t));
		f.width = -1;
		f.prec = -1;
		f.len = L_none;

		// (no positional 'n$' support)

		// flags
		p = parse_flags(p, &f.flags);

		// width
		p = parse_width_prec(p, &f.width, &ap);
		// if width came from '*' and was negative -> '-' flag and positive width
		if (f.width < 0 && * (p-1) == '*') {
			f.flags |= F_LEFT;
			f.width = -f.width;
		}

		// precision
		if (*p == '.') {
			p++;
			if (*p == '*') {
				v = va_arg(ap, int);
				f.prec = (v < 0) ? -1 : v;
				p++;
			} else {
				v = 0;
				any = 0;
				while (is_digit(*p)) {
					any = 1;
					v = v * 10 + (*p - '0');
					p++;
				}
				f.prec = any ? v : 0;
			}
		}

		// length
		p = parse_len(p, (int*)&f.len);

		// type
		f.type = *p ? *p++ : '\0';
		if (!f.type)
			break;

		switch (f.type) {
		case '%':
			out_putc(&out, '%');
			break;

		case 'c': {
			// length modifiers ignored for %c
			ch = (char)va_arg(ap, int);
			pad_len = (f.width > 1) ? (f.width - 1) : 0;
			if (!(f.flags & F_LEFT))
				pad(&out, ' ', pad_len);
			out_putc(&out, ch);
			if (f.flags & F_LEFT)
				pad(&out, ' ', pad_len);
		} break;

		case 's': {
			const char *s = va_arg(ap, const char*);
			if (!s)
				s = "(null)";
			sl = cstr_len(s);
			if (f.prec >= 0 && (size_t)f.prec < sl)
				sl = (size_t)f.prec;
			pad_len = (f.width > (int)sl) ? f.width - (int)sl : 0;
			if (!(f.flags & F_LEFT))
				pad(&out, ' ', pad_len);
			out_write_n(&out, s, sl);
			if (f.flags & F_LEFT)
				pad(&out, ' ', pad_len);
		} break;

		case 'd': case 'i': {
			v = get_int_arg(&ap, f.len);
			u = (v < 0) ? (uint64_t)(-v) : (uint64_t)v;
			if (f.prec >= 0)
				f.flags &= ~F_ZERO;
			emit_int(&out, &f, u, v < 0, 10, 0);
		} break;

		case 'u': {
			u = get_uint_arg(&ap, f.len);
			if (f.prec >= 0)
				f.flags &= ~F_ZERO;
			emit_int(&out, &f, u, 0, 10, 0);
		} break;

		case 'o': {
			u = get_uint_arg(&ap, f.len);
			if (f.prec >= 0)
				f.flags &= ~F_ZERO;
			emit_int(&out, &f, u, 0, 8, 0);
		} break;

		case 'x': case 'X': {
			u = get_uint_arg(&ap, f.len);
			if (f.prec >= 0)
				f.flags &= ~F_ZERO;
			emit_int(&out, &f, u, 0, 16, (f.type == 'X'));
		} break;

		case 'p': {
			// implementation-defined; we print 0x... with fixed width of pointer
			pt = (uintptr_t)va_arg(ap, void*);
			g = f;
			g.flags |= F_ALT;
			g.prec = (int)(sizeof(void*) * 2);
			g.flags &= ~(F_SIGN | F_SP);
			emit_int(&out, &g, (uint64_t)pt, 0, 16, 0);
		} break;

		case 'n': {
			// store number of chars written so far (not counting terminating NUL)
			// respect length modifier
			written = out.len;
			switch (f.len) {
			case L_hh: *va_arg(ap, signed char*) = (signed char)written; break;
			case L_h:  *va_arg(ap, short*)       = (short)written; break;
			case L_l:  *va_arg(ap, long*)        = (long)written; break;
			case L_ll: *va_arg(ap, long long*)   = (long long)written; break;
			case L_j:  *va_arg(ap, intmax_t*)    = (intmax_t)written; break;
			case L_z:  *va_arg(ap, size_t*)      = (size_t)written; break;
			case L_t:  *va_arg(ap, ptrdiff_t*)   = (ptrdiff_t)written; break;
			default:   *va_arg(ap, int*)         = (int)written; break;
			}
		} break;

		case 'f': case 'F':
			emit_float_f(&out, &f, va_arg(ap, double), (f.type == 'F'));
			break;
		case 'e': case 'E':
			emit_float_e(&out, &f, va_arg(ap, double), (f.type == 'E'));
			break;
		case 'g': case 'G':
			emit_float_g(&out, &f, va_arg(ap, double), (f.type == 'G'));
			break;

		default:
			// unknown specifier: print literally
			out_putc(&out, '%');
			out_putc(&out, f.type);
			break;
		}
	}

	out_terminate(&out);
	return (int)out.len;
}

int snprintf(char *dst, size_t n, const char *fmt, ...)
{
	va_list ap;
	int r;
	va_start(ap, fmt);
	r = vsnprintf(dst, n, fmt, ap);
	va_end(ap);
	return r;
}

