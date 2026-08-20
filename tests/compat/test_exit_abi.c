#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef void (*exit_function)(int status);

static int callback_fd = -1;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "_exit ABI test failed at line %d\n", __LINE__); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static void exit_callback(void) {
    static const char marker = 'x';

    if (callback_fd >= 0)
        (void)write(callback_fd, &marker, sizeof(marker));
}

static int verify_status(exit_function function, int requested_status) {
    char marker;
    int pipe_fds[2] = {-1, -1};
    int status;
    pid_t child;
    ssize_t count;

    CHECK(pipe(pipe_fds) == 0);
    child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        close(pipe_fds[0]);
        callback_fd = pipe_fds[1];
        if (atexit(exit_callback) != 0)
            _Exit(126);
        function(requested_status);
        _Exit(125);
    }

    CHECK(close(pipe_fds[1]) == 0);
    pipe_fds[1] = -1;
    CHECK(waitpid(child, &status, 0) == child);
    do {
        count = read(pipe_fds[0], &marker, sizeof(marker));
    } while (count < 0 && errno == EINTR);
    CHECK(count == 0);
    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == (unsigned char)requested_status);
    CHECK(close(pipe_fds[0]) == 0);
    return 0;
}

int main(void) {
    static const int requested_statuses[] = {0, 42, 0x1234, -1};
    exit_function function = _exit;
    Dl_info info;
    size_t index;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "_exit") == (void*)function);

    for (index = 0; index < sizeof(requested_statuses) / sizeof(requested_statuses[0]); ++index)
        CHECK(verify_status(function, requested_statuses[index]) == 0);
    return 0;
}
