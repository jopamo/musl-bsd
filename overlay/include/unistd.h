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

#ifdef __cplusplus
extern "C" {
#endif

off64_t lseek64(int fd, off64_t offset, int whence);
ssize_t pread64(int fd, void* buf, size_t count, off64_t offset);
ssize_t pwrite64(int fd, const void* buf, size_t count, off64_t offset);

#ifdef __cplusplus
}
#endif

#endif
