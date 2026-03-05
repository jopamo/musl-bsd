#ifndef _CDEFS_H_
#define _CDEFS_H_

#include <alloca.h>
#include <errno.h>
#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)
#endif
#ifndef ALLPERMS
#define ALLPERMS (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO)
#endif
#ifndef DEFFILEMODE
#define DEFFILEMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#endif

#ifndef FNM_EXTMATCH
#define FNM_EXTMATCH 0
#endif

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression)               \
    (__extension__({                                 \
        long int __result;                           \
        do {                                         \
            __result = (long int)(expression);       \
        } while (__result == -1L && errno == EINTR); \
        __result;                                    \
    }))
#endif

#if !defined(HAVE_QSORT_R)

typedef int (*qsort_r_compar_fn_t)(const void*, const void*, void*);

struct qsort_r_shim_ctx {
    qsort_r_compar_fn_t compar;
    void* arg;
    struct qsort_r_shim_ctx* prev;
};

static __thread struct qsort_r_shim_ctx* qsort_r_ctx_top;

static int qsort_r_global_shim(const void* a, const void* b) {
    struct qsort_r_shim_ctx* ctx = qsort_r_ctx_top;
    return ctx->compar(a, b, ctx->arg);
}

#define qsort_r(base, nmemb, size, cmpfn, user_arg)          \
    do {                                                     \
        struct qsort_r_shim_ctx __ctx = {                    \
            .compar = (qsort_r_compar_fn_t)(cmpfn),          \
            .arg = (void*)(user_arg),                        \
            .prev = qsort_r_ctx_top,                         \
        };                                                   \
        qsort_r_ctx_top = &__ctx;                            \
        qsort((base), (nmemb), (size), qsort_r_global_shim); \
        qsort_r_ctx_top = __ctx.prev;                        \
    } while (0)

#endif

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
#ifndef __BEGIN_EXTERN_C
#define __BEGIN_EXTERN_C extern "C" {
#endif
#ifndef __END_EXTERN_C
#define __END_EXTERN_C }
#endif
#else
#ifndef __BEGIN_EXTERN_C
#define __BEGIN_EXTERN_C
#endif
#ifndef __END_EXTERN_C
#define __END_EXTERN_C
#endif
#endif

#ifndef __BEGIN_DECLS
#define __BEGIN_DECLS __BEGIN_EXTERN_C
#endif
#ifndef __END_DECLS
#define __END_DECLS __END_EXTERN_C
#endif

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

#endif
