#ifndef MUSL_BSD_OVERLAY_FCNTL_H
#include_next <fcntl.h>

#define MUSL_BSD_OVERLAY_FCNTL_H

#ifdef __cplusplus
extern "C" {
#endif

int open64(const char* path, int oflag, ...);

#ifdef __cplusplus
}
#endif

#endif
