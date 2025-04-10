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

#ifndef strndupa
#define strndupa(s, n)                         \
    (__extension__({                           \
        const char* __in = (s);                \
        size_t __len = strnlen(__in, (n)) + 1; \
        char* __out = (char*)alloca(__len);    \
        __out[__len - 1] = '\0';               \
        (char*)memcpy(__out, __in, __len - 1); \
    }))
#endif

#ifndef mempcpy
#define mempcpy(to, from, size)     \
    (__extension__({                \
        void* __dst = (to);         \
        const void* __src = (from); \
        size_t __sz = (size);       \
        memcpy(__dst, __src, __sz); \
        (char*)__dst + __sz;        \
    }))
#endif

#ifndef strcasecmp
#define strcasecmp(_l, _r)                                                         \
    (__extension__({                                                               \
        const unsigned char* __l = (const unsigned char*)(_l);                     \
        const unsigned char* __r = (const unsigned char*)(_r);                     \
        while (*__l && *__r && (*__l == *__r || tolower(*__l) == tolower(*__r))) { \
            __l++;                                                                 \
            __r++;                                                                 \
        }                                                                          \
        tolower(*__l) - tolower(*__r);                                             \
    }))
#endif

#ifndef strchrnul
#define strchrnul(p, c)                    \
    (__extension__({                       \
        const char* __str = (p);           \
        int __ch = (c);                    \
        while (*__str && (*__str != __ch)) \
            __str++;                       \
        (char*)__str;                      \
    }))
#endif

#ifndef strndup
#define strndup(s, n)                             \
    (__extension__({                              \
        const char* __src = (s);                  \
        size_t __size = (n);                      \
        char* __end = memchr(__src, 0, __size);   \
        if (__end)                                \
            __size = (size_t)(__end - __src + 1); \
        char* __out = (char*)malloc(__size);      \
        if (__out && __size > 0) {                \
            memcpy(__out, __src, __size - 1);     \
            __out[__size - 1] = '\0';             \
        }                                         \
        __out;                                    \
    }))
#endif

#if !defined(HAVE_QSORT_R)

static void* qsort_r_global_arg;

typedef int (*qsort_r_compar_fn_t)(const void*, const void*, void*);
static qsort_r_compar_fn_t qsort_r_user_compar;

static int qsort_r_global_shim(const void* a, const void* b) {
    return qsort_r_user_compar(a, b, qsort_r_global_arg);
}

#define qsort_r(base, nmemb, size, compar, arg)              \
    do {                                                     \
        qsort_r_user_compar = (qsort_r_compar_fn_t)(compar); \
        qsort_r_global_arg = (void*)(arg);                   \
        qsort((base), (nmemb), (size), qsort_r_global_shim); \
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

#endif
