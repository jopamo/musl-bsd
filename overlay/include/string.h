#ifndef MUSL_BSD_OVERLAY_STRING_H
#include_next <string.h>

#define MUSL_BSD_OVERLAY_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);

#ifdef __cplusplus
}
#endif

#endif
