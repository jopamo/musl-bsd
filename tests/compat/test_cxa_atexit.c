#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern int __cxa_atexit(void (*function)(void*), void* argument, void* dso);

enum {
    REGISTRATION_COUNT = 97,
    CHILD_EXIT_STATUS = 37,
};

struct callback_record {
    int fd;
    unsigned char value;
};

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "__cxa_atexit test failed at line %d\n", __LINE__); \
            return 1;                                                           \
        }                                                                       \
    } while (0)

static void record_callback(void* argument) {
    const struct callback_record* record = argument;
    ssize_t written;

    do {
        written = write(record->fd, &record->value, 1);
    } while (written < 0 && errno == EINTR);
    if (written != 1)
        _Exit(121);
}

static ssize_t read_callbacks(int fd, unsigned char* values, size_t capacity) {
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
    int (*cxa_atexit)(void (*)(void*), void*, void*) = __cxa_atexit;
    static struct callback_record records[REGISTRATION_COUNT];
    static int dso_tokens[2];
    unsigned char values[REGISTRATION_COUNT + 1];
    int pipe_fds[2];
    int status;
    Dl_info info;
    pid_t child;
    size_t index;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)cxa_atexit, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__cxa_atexit") == (void*)cxa_atexit);

    CHECK(pipe(pipe_fds) == 0);
    child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        close(pipe_fds[0]);
        for (index = 0; index < REGISTRATION_COUNT; ++index) {
            records[index].fd = pipe_fds[1];
            records[index].value = (unsigned char)index;
            errno = EDOM;
            if (cxa_atexit(record_callback, &records[index], &dso_tokens[index % 2]) != 0 || errno != EDOM)
                _Exit(120);
        }
        exit(CHILD_EXIT_STATUS);
    }

    CHECK(close(pipe_fds[1]) == 0);
    CHECK(read_callbacks(pipe_fds[0], values, sizeof(values)) == REGISTRATION_COUNT);
    CHECK(close(pipe_fds[0]) == 0);
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == CHILD_EXIT_STATUS);

    for (index = 0; index < REGISTRATION_COUNT; ++index)
        CHECK(values[index] == REGISTRATION_COUNT - index - 1);
    return 0;
}
