#ifndef MUSL_BSD_OVERLAY_SYS_WAIT_H
#include_next <sys/wait.h>

#define MUSL_BSD_OVERLAY_SYS_WAIT_H

#ifndef W_EXITCODE
#define W_EXITCODE(ret, sig) (((ret) << 8) | (sig))
#endif

#endif
