#ifndef MUSL_BSD_OVERLAY_STDIO_H
#include_next <stdio.h>

#define MUSL_BSD_OVERLAY_STDIO_H

#include <sys/types.h>

#ifndef fopen64
#define fopen64 fopen
#endif

#ifndef fseeko64
#define fseeko64 fseeko
#endif

#ifndef ftello64
#define ftello64 ftello
#endif

#endif
