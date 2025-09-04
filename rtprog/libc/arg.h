
// Author: Gustaf Franzen <gustaffranzen@icloud.com>
#ifndef _STDARG_H_
#define _STDARG_H_

#if !defined(__GNUC__) && !defined(__clang__)
#	error "stdarg.h requires compiler varargs builtins (__builtin_*)."
#endif

typedef __builtin_va_list va_list;

#define va_start(ap, last)	__builtin_va_start((ap), (last))
#define va_end(ap)		__builtin_va_end((ap))
#define va_arg(ap, type)	__builtin_va_arg((ap), type)
#define va_copy(dst, src)	__builtin_va_copy((dst), (src))

#endif /* _STDARG_H */
