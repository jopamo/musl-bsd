#ifndef MUSL_BSD_OVERLAY_SYS_MMAN_H
#include_next <sys/mman.h>

#define MUSL_BSD_OVERLAY_SYS_MMAN_H

#include <sys/types.h>

#ifndef mmap64
#define mmap64 mmap
#endif

#endif
