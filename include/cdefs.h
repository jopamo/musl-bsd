#ifndef _CDEFS_H_
#define _CDEFS_H_

#define __dead __attribute__((__noreturn__))
#define __pure __attribute__((__const__))
#define __used __attribute__((__used__))
#define __warn_unused_result __attribute__((__warn_unused_result__))
#define __returns_twice __attribute__((returns_twice))
#define __only_inline extern __inline __attribute__((__gnu_inline__))

#define __predict_true(expr) __builtin_expect(!!(expr), 1)
#define __predict_false(expr) __builtin_expect(!!(expr), 0)

#define __packed __attribute__((__packed__))
#define __aligned(x) __attribute__((__aligned__(x)))
#define __malloc __attribute__((__malloc__))

#ifndef __extension__
#define __extension__
#endif

#if defined(__cplusplus)
#define __BEGIN_EXTERN_C extern "C" {
#define __END_EXTERN_C }
#else
#define __BEGIN_EXTERN_C
#define __END_EXTERN_C
#endif

#define __BEGIN_DECLS __BEGIN_EXTERN_C
#define __END_DECLS __END_EXTERN_C

#define __dso_public __attribute__((__visibility__("default")))
#define __dso_hidden __attribute__((__visibility__("hidden")))

#define __BEGIN_PUBLIC_DECLS _Pragma("GCC visibility push(default)") __BEGIN_EXTERN_C
#define __END_PUBLIC_DECLS __END_EXTERN_C _Pragma("GCC visibility pop")
#define __BEGIN_HIDDEN_DECLS _Pragma("GCC visibility push(hidden)") __BEGIN_EXTERN_C
#define __END_HIDDEN_DECLS __END_EXTERN_C _Pragma("GCC visibility pop")

#ifndef __XPG_VISIBLE
#define __XPG_VISIBLE 700
#endif

#ifndef __POSIX_VISIBLE
#define __POSIX_VISIBLE 200809L
#endif

#ifndef __ISO_C_VISIBLE
#define __ISO_C_VISIBLE 2011
#endif

#ifndef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#endif

#endif /* _CDEFS_H_ */
