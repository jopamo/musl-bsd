#include <sys/pidfd.h>

int main(void) {
    int (*open_function)(pid_t, unsigned int) = pidfd_open;
    int (*getfd_function)(int, int, unsigned int) = pidfd_getfd;
    int (*signal_function)(int, int, siginfo_t*, unsigned int) = pidfd_send_signal;

    return open_function == 0 || getfd_function == 0 || signal_function == 0;
}
