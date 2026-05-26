#ifndef MUSL_BSD_OVERLAY_SYS_MMAN_H
#include_next <sys/mman.h>

#define MUSL_BSD_OVERLAY_SYS_MMAN_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void* mmap64(void* addr, size_t length, int prot, int flags, int fd, off64_t offset);

#ifdef __cplusplus
}
#endif

#endif
