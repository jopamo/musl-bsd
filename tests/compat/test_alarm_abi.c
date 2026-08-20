#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef unsigned (*alarm_function)(unsigned seconds);

static volatile sig_atomic_t alarm_seen;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "alarm ABI test failed at line %d\n", __LINE__); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static void alarm_handler(int signal_number) {
    (void)signal_number;
    alarm_seen = 1;
}

static int verify_alarm(alarm_function function) {
    struct sigaction action = {
        .sa_handler = alarm_handler,
    };
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 10000000,
    };
    unsigned previous;

    CHECK(sigemptyset(&action.sa_mask) == 0);
    CHECK(sigaction(SIGALRM, &action, NULL) == 0);

    errno = EDOM;
    CHECK(function(0) == 0);
    CHECK(errno == EDOM);
    errno = ERANGE;
    CHECK(function(2) == 0);
    CHECK(errno == ERANGE);
    errno = E2BIG;
    previous = function(0);
    CHECK(previous == 2);
    CHECK(errno == E2BIG);

    alarm_seen = 0;
    errno = ENOTTY;
    CHECK(function(1) == 0);
    CHECK(errno == ENOTTY);
    for (int attempt = 0; attempt < 200 && !alarm_seen; ++attempt)
        nanosleep(&delay, NULL);
    CHECK(alarm_seen != 0);
    errno = ENOEXEC;
    CHECK(function(0) == 0);
    CHECK(errno == ENOEXEC);
    return 0;
}

int main(void) {
    alarm_function function = alarm;
    int status;
    pid_t child;
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "alarm") == (void*)function);

    child = fork();
    CHECK(child >= 0);
    if (child == 0)
        _exit(verify_alarm(function) == 0 ? 0 : 120);
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
    return 0;
}
