#include <sys/pidfd.h>

#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int pidfd_open(pid_t pid, unsigned int flags) {
#ifdef SYS_pidfd_open
    return syscall(SYS_pidfd_open, pid, flags);
#else
    errno = ENOSYS;
    return -1;
#endif
}

int pidfd_getfd(int pidfd, int targetfd, unsigned int flags) {
#ifdef SYS_pidfd_getfd
    return syscall(SYS_pidfd_getfd, pidfd, targetfd, flags);
#else
    errno = ENOSYS;
    return -1;
#endif
}

int pidfd_send_signal(int pidfd, int sig, siginfo_t* info, unsigned int flags) {
#ifdef SYS_pidfd_send_signal
    return syscall(SYS_pidfd_send_signal, pidfd, sig, info, flags);
#else
    errno = ENOSYS;
    return -1;
#endif
}
