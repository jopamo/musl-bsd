#ifndef _CDEFS_H_
#define _CDEFS_H_

/*
 * cdefs.h - Simplified "BSD-style" compiler definitions for musl + GCC >= 13 or Clang >= 17.

/* Function does not return */
#define __dead __attribute__((__noreturn__))

/* Function has no side effects except its return value */
#define __pure __attribute__((__const__))

/* Mark variable/function as possibly unused to silence warnings */
#define __unused __attribute__((__unused__))

/* Ensure an entity is emitted even if not referenced */
#define __used __attribute__((__used__))

/* Warn if the result of a function call is ignored */
#define __warn_unused_result __attribute__((__warn_unused_result__))

/* e.g. setjmp/longjmp usage – function can return multiple times */
#define __returns_twice __attribute__((returns_twice))

/* For inline functions that should not produce an external out-of-line copy. */
#define __only_inline extern __inline __attribute__((__gnu_inline__))

/*
 * Branch prediction hints:
 *   __predict_true(expr)   => compiler optimizes for (expr != 0)
 *   __predict_false(expr)  => compiler optimizes for (expr == 0)
 */
#define __predict_true(expr) __builtin_expect(!!(expr), 1)
#define __predict_false(expr) __builtin_expect(!!(expr), 0)

/*
 * __packed => pack structure members with minimal alignment
 * __aligned(x) => specify alignment x for the entity
 */
#define __packed __attribute__((__packed__))
#define __aligned(x) __attribute__((__aligned__(x)))

/* Function returns a pointer not aliased by any other pointer */
#define __malloc __attribute__((__malloc__))

/* For code that triggers certain GNU extensions to avoid pedantic warnings */
#ifndef __extension__
#define __extension__ /* normally defined by GCC; ensure no error if missing */
#endif

/* ---------------------------------------------------------------------
 * extern "C" macros for C++ interop
 * --------------------------------------------------------------------- */
#if defined(__cplusplus)
#define __BEGIN_EXTERN_C extern "C" {
#define __END_EXTERN_C }
#else
#define __BEGIN_EXTERN_C
#define __END_EXTERN_C
#endif

/*
 * Historically in BSD code, these are synonyms:
 */
#define __BEGIN_DECLS __BEGIN_EXTERN_C
#define __END_DECLS __END_EXTERN_C

/* ---------------------------------------------------------------------
 * Visibility attributes (for shared libraries)
 * --------------------------------------------------------------------- */
#define __dso_public __attribute__((__visibility__("default")))
#define __dso_hidden __attribute__((__visibility__("hidden")))

#define __BEGIN_PUBLIC_DECLS _Pragma("GCC visibility push(default)") __BEGIN_EXTERN_C
#define __END_PUBLIC_DECLS __END_EXTERN_C _Pragma("GCC visibility pop")

#define __BEGIN_HIDDEN_DECLS _Pragma("GCC visibility push(hidden)") __BEGIN_EXTERN_C
#define __END_HIDDEN_DECLS __END_EXTERN_C _Pragma("GCC visibility pop")

/* ---------------------------------------------------------------------
 * Feature‐test macros
 *
 * Many BSD-style headers rely on macros like __POSIX_VISIBLE, __XPG_VISIBLE, etc.
 * On musl, typically _GNU_SOURCE or _DEFAULT_SOURCE is used.
 * We define some "new enough" defaults so that legacy BSD code compiles.
 * You can override them with e.g. -D_POSIX_C_SOURCE=200809L, etc.
 * --------------------------------------------------------------------- */
#ifndef __XPG_VISIBLE
#define __XPG_VISIBLE 700 /* e.g. XPG7 */
#endif

#ifndef __POSIX_VISIBLE
#define __POSIX_VISIBLE 200809L /* e.g. POSIX.1-2008 */
#endif

#ifndef __ISO_C_VISIBLE
#define __ISO_C_VISIBLE 2011 /* e.g. C11 */
#endif

#ifndef __BSD_VISIBLE
#define __BSD_VISIBLE 1 /* expose BSD extensions by default */
#endif

#endif /* _CDEFS_H_ */
