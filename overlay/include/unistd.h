#ifndef MUSL_BSD_OVERLAY_UNISTD_H
#include_next <unistd.h>

#define MUSL_BSD_OVERLAY_UNISTD_H

#include <errno.h>
#include <sys/types.h>

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

#ifndef lseek64
#define lseek64 lseek
#endif

#ifndef pread64
#define pread64 pread
#endif

#ifndef pwrite64
#define pwrite64 pwrite
#endif

#endif
