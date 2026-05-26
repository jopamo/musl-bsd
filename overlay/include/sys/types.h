#ifndef MUSL_BSD_OVERLAY_SYS_TYPES_H
#include_next <sys/types.h>

#define MUSL_BSD_OVERLAY_SYS_TYPES_H

#ifndef off64_t
#define off64_t off_t
#endif

#endif
