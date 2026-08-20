#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

extern int __libc_current_sigrtmin(void);

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "sigrtmin test failed at line %d\n", __LINE__); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

static volatile sig_atomic_t received_signal;

static void signal_handler(int signal, siginfo_t* information, void* context) {
    (void)information;
    (void)context;
    received_signal = signal;
}

static int verify_signal_operations(int signal) {
    static const int queued_value = 0x1357;
    struct sigaction action;
    struct sigaction previous_action;
    siginfo_t information;
    sigset_t blocked;
    sigset_t delivery_mask;
    sigset_t previous_mask;
    union sigval value;
    long thread_id;

    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_handler;
    action.sa_flags = SA_SIGINFO;
    CHECK(sigemptyset(&action.sa_mask) == 0);
    CHECK(sigaction(signal, &action, &previous_action) == 0);

    CHECK(sigemptyset(&blocked) == 0);
    CHECK(sigaddset(&blocked, signal) == 0);
    CHECK(sigismember(&blocked, signal) == 1);
    CHECK(sigprocmask(SIG_BLOCK, &blocked, &previous_mask) == 0);

    value.sival_int = queued_value;
    CHECK(sigqueue(getpid(), signal, value) == 0);
    memset(&information, 0, sizeof(information));
    CHECK(sigwaitinfo(&blocked, &information) == signal);
    CHECK(information.si_signo == signal);
    CHECK(information.si_code == SI_QUEUE);
    CHECK(information.si_value.sival_int == queued_value);

    delivery_mask = previous_mask;
    CHECK(sigdelset(&delivery_mask, signal) == 0);
    CHECK(sigprocmask(SIG_SETMASK, &delivery_mask, NULL) == 0);
    received_signal = 0;
    thread_id = syscall(SYS_gettid);
    CHECK(thread_id > 0);
    CHECK(syscall(SYS_tgkill, getpid(), thread_id, signal) == 0);
    CHECK(received_signal == signal);

    CHECK(sigprocmask(SIG_SETMASK, &previous_mask, NULL) == 0);
    CHECK(sigaction(signal, &previous_action, NULL) == 0);
    return 0;
}

int main(void) {
    int (*current_sigrtmin)(void) = __libc_current_sigrtmin;
    int signal;
    Dl_info info;

    _Static_assert(sizeof(signal) == 4, "x86_64 glibc signal-number ABI");

    errno = EDOM;
    signal = current_sigrtmin();
    CHECK(signal == 35);
    CHECK(errno == EDOM);
    errno = ERANGE;
    CHECK(current_sigrtmin() == signal);
    CHECK(SIGRTMIN == signal);
    CHECK(SIGRTMAX == 64);
    CHECK(errno == ERANGE);
    CHECK(signal <= SIGRTMAX);

    CHECK(verify_signal_operations(signal) == 0);

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)current_sigrtmin, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__libc_current_sigrtmin") == (void*)current_sigrtmin);
    return 0;
}
