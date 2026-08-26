#include <sys/pidfd.h>

#include <errno.h>
#include <unistd.h>

int main(void) {
    int fd = pidfd_open(getpid(), 0);

    if (fd >= 0)
        return close(fd) == 0 ? 0 : 1;
    return errno == ENOSYS ? 0 : 1;
}
