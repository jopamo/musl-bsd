#ifndef MUSL_BSD_OVERLAY_FCNTL_H
#include_next <fcntl.h>

#define MUSL_BSD_OVERLAY_FCNTL_H

#ifndef open64
#define open64 open
#endif

#endif
