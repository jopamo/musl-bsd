#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern int __cxa_atexit(void (*function)(void*), void* argument, void* dso);
extern void __cxa_finalize(void* dso);

enum {
    MARKER_DSO_FINALIZED = 0xf0,
    MARKER_ALL_FINALIZED = 0xf1,
    CALLBACK_A = 0xa0,
    CALLBACK_B = 0xb0,
    CHILD_EXIT_STATUS = 38,
};

struct callback_record {
    int fd;
    unsigned char value;
};

#define CHECK(condition)                                                          \
    do {                                                                          \
        if (!(condition)) {                                                       \
            fprintf(stderr, "__cxa_finalize test failed at line %d\n", __LINE__); \
            return 1;                                                             \
        }                                                                         \
    } while (0)

static void write_value(int fd, unsigned char value) {
    ssize_t written;

    do {
        written = write(fd, &value, 1);
    } while (written < 0 && errno == EINTR);
    if (written != 1)
        _Exit(121);
}

static void record_callback(void* argument) {
    const struct callback_record* record = argument;

    write_value(record->fd, record->value);
}

static ssize_t read_trace(int fd, unsigned char* values, size_t capacity) {
    size_t length = 0;

    while (length < capacity) {
        ssize_t count = read(fd, values + length, capacity - length);

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
    return (ssize_t)length;
}

int main(void) {
    void (*cxa_finalize)(void*) = __cxa_finalize;
    struct callback_record record_a;
    struct callback_record record_b;
    unsigned char values[5];
    int dso_a;
    int dso_b;
    int pipe_fds[2];
    int status;
    Dl_info info;
    pid_t child;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)cxa_finalize, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__cxa_finalize") == (void*)cxa_finalize);

    CHECK(pipe(pipe_fds) == 0);
    child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        close(pipe_fds[0]);
        record_a = (struct callback_record){pipe_fds[1], CALLBACK_A};
        record_b = (struct callback_record){pipe_fds[1], CALLBACK_B};
        if (__cxa_atexit(record_callback, &record_a, &dso_a) != 0 ||
            __cxa_atexit(record_callback, &record_b, &dso_b) != 0)
            _Exit(120);

        errno = EDOM;
        cxa_finalize(&dso_a);
        if (errno != EDOM)
            _Exit(122);
        write_value(pipe_fds[1], MARKER_DSO_FINALIZED);

        errno = ERANGE;
        cxa_finalize(NULL);
        if (errno != ERANGE)
            _Exit(123);
        write_value(pipe_fds[1], MARKER_ALL_FINALIZED);

        errno = EILSEQ;
        cxa_finalize(&dso_a);
        if (errno != EILSEQ)
            _Exit(124);
        exit(CHILD_EXIT_STATUS);
    }

    CHECK(close(pipe_fds[1]) == 0);
    CHECK(read_trace(pipe_fds[0], values, sizeof(values)) == 4);
    CHECK(close(pipe_fds[0]) == 0);
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == CHILD_EXIT_STATUS);

    CHECK(values[0] == MARKER_DSO_FINALIZED);
    CHECK(values[1] == MARKER_ALL_FINALIZED);
    CHECK(values[2] == CALLBACK_B);
    CHECK(values[3] == CALLBACK_A);
    return 0;
}
