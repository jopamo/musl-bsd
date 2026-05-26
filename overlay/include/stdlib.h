#ifndef MUSL_BSD_OVERLAY_STDLIB_H
#include_next <stdlib.h>

#define MUSL_BSD_OVERLAY_STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

char* secure_getenv(const char* name);

#ifdef __cplusplus
}
#endif

#endif
