#ifndef MUSL_BSD_OVERLAY_SYS_PIDFD_H
#define MUSL_BSD_OVERLAY_SYS_PIDFD_H

#include <signal.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int pidfd_open(pid_t pid, unsigned int flags);
int pidfd_getfd(int pidfd, int targetfd, unsigned int flags);
int pidfd_send_signal(int pidfd, int sig, siginfo_t* info, unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif
