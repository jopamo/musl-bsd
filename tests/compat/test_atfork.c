#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern int __register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void), void* dso_handle);

static int parent_events = -1;
static int child_events = -1;
static pid_t owner_pid;

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "atfork ABI test failed at line %d\n", __LINE__); \
            return 1;                                                         \
        }                                                                     \
    } while (0)

static void emit_event(unsigned char event) {
    int fd = getpid() == owner_pid ? parent_events : child_events;
    ssize_t result;

    do {
        result = write(fd, &event, sizeof(event));
    } while (result < 0 && errno == EINTR);
    if (result != (ssize_t)sizeof(event))
        _exit(120);
}

static ssize_t read_exact(int fd, unsigned char* buffer, size_t length) {
    size_t offset = 0;

    while (offset < length) {
        ssize_t result = read(fd, buffer + offset, length - offset);

        if (result > 0) {
            offset += (size_t)result;
            continue;
        }
        if (result < 0 && errno == EINTR)
            continue;
        return result;
    }
    return (ssize_t)offset;
}

static void prepare_first(void) {
    emit_event('1');
}

static void parent_first(void) {
    emit_event('3');
}

static void child_first(void) {
    emit_event('5');
}

static void prepare_second(void) {
    emit_event('2');
}

static void parent_second(void) {
    emit_event('4');
}

static void child_second(void) {
    emit_event('6');
}

int main(void) {
    int (*register_atfork)(void (*)(void), void (*)(void), void (*)(void), void*) = __register_atfork;
    static int dso_tokens[2];
    unsigned char expected_parent[] = {'2', '1', '3', '4'};
    unsigned char expected_child[] = {'5', '6', '7'};
    unsigned char parent_buffer[sizeof(expected_parent)];
    unsigned char child_buffer[sizeof(expected_child)];
    int parent_pipe[2];
    int child_pipe[2];
    int status;
    Dl_info info;
    pid_t child;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)register_atfork, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") != NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__register_atfork") == (void*)register_atfork);

    CHECK(pipe(parent_pipe) == 0);
    CHECK(pipe(child_pipe) == 0);
    owner_pid = getpid();
    parent_events = parent_pipe[1];
    child_events = child_pipe[1];

    errno = EDOM;
    CHECK(register_atfork(prepare_first, parent_first, child_first, &dso_tokens[0]) == 0);
    CHECK(errno == EDOM);
    errno = ERANGE;
    CHECK(register_atfork(prepare_second, parent_second, child_second, &dso_tokens[1]) == 0);
    CHECK(errno == ERANGE);

    child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        close(parent_pipe[0]);
        close(parent_pipe[1]);
        close(child_pipe[0]);
        emit_event('7');
        close(child_pipe[1]);
        _exit(0);
    }

    CHECK(close(parent_pipe[1]) == 0);
    CHECK(close(child_pipe[1]) == 0);
    CHECK(read_exact(parent_pipe[0], parent_buffer, sizeof(parent_buffer)) == (ssize_t)sizeof(parent_buffer));
    CHECK(read_exact(child_pipe[0], child_buffer, sizeof(child_buffer)) == (ssize_t)sizeof(child_buffer));
    CHECK(close(parent_pipe[0]) == 0);
    CHECK(close(child_pipe[0]) == 0);
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 0);
    CHECK(memcmp(parent_buffer, expected_parent, sizeof(parent_buffer)) == 0);
    CHECK(memcmp(child_buffer, expected_child, sizeof(child_buffer)) == 0);
    return 0;
}
