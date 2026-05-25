#ifndef MUSL_BSD_OVERLAY_SYS_WAIT_H
#define MUSL_BSD_OVERLAY_SYS_WAIT_H

#include_next <sys/wait.h>

#ifndef W_EXITCODE
#define W_EXITCODE(ret, sig) (((ret) << 8) | (sig))
#endif

#endif
