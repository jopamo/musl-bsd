#ifndef MUSL_BSD_OVERLAY_STDIO_H
#include_next <stdio.h>

#define MUSL_BSD_OVERLAY_STDIO_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

FILE* fopen64(const char* path, const char* mode);
int fseeko64(FILE* stream, off64_t offset, int whence);
off64_t ftello64(FILE* stream);

#ifdef __cplusplus
}
#endif

#endif
