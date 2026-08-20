#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

_Noreturn void __assert_fail(const char* assertion, const char* file, unsigned int line, const char* function);

#define CHECK(condition)                                                         \
    do {                                                                         \
        if (!(condition)) {                                                      \
            fprintf(stderr, "__assert_fail test failed at line %d\n", __LINE__); \
            return 1;                                                            \
        }                                                                        \
    } while (0)

static ssize_t read_diagnostic(int fd, char* buffer, size_t capacity) {
    size_t length = 0;

    while (length + 1 < capacity) {
        ssize_t count = read(fd, buffer + length, capacity - length - 1);

        if (count > 0) {
            length += (size_t)count;
            continue;
        }
        if (count < 0 && errno == EINTR)
            continue;
        if (count < 0)
            return -1;
        break;
    }
    buffer[length] = '\0';
    return (ssize_t)length;
}

int main(void) {
    void (*assert_fail)(const char*, const char*, unsigned int, const char*) = __assert_fail;
    char diagnostic[512];
    int pipe_fds[2];
    int status;
    Dl_info info;
    pid_t child;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)assert_fail, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__assert_fail") == (void*)assert_fail);

    CHECK(pipe(pipe_fds) == 0);
    child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        close(pipe_fds[0]);
        if (dup2(pipe_fds[1], STDERR_FILENO) < 0)
            _exit(120);
        close(pipe_fds[1]);
        signal(SIGABRT, SIG_DFL);
        assert_fail("gpu != NULL", "nvidia-test.c", 321, "load_gpu");
    }

    CHECK(close(pipe_fds[1]) == 0);
    CHECK(read_diagnostic(pipe_fds[0], diagnostic, sizeof(diagnostic)) > 0);
    CHECK(close(pipe_fds[0]) == 0);
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFSIGNALED(status));
    CHECK(WTERMSIG(status) == SIGABRT);
    CHECK(strstr(diagnostic, "gpu != NULL") != NULL);
    CHECK(strstr(diagnostic, "nvidia-test.c") != NULL);
    CHECK(strstr(diagnostic, "load_gpu") != NULL);
    CHECK(strstr(diagnostic, "321") != NULL);
    return 0;
}
