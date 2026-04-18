#include <obstack.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_child_exit(pid_t pid) {
    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    return WEXITSTATUS(status);
}

static void test_default_alloc_failed_handler_exits_with_configured_code(void) {
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        obstack_exit_failure = 77;
        (*obstack_alloc_failed_handler)();
        _exit(0);
    }
    int code = wait_child_exit(pid);
    assert(code == 77);
}

static void test_xmalloc_failed_exits_with_configured_code(void) {
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        obstack_exit_failure = 78;
        xmalloc_failed(123);
        _exit(0);
    }
    int code = wait_child_exit(pid);
    assert(code == 78);
}

static void test_xmalloc_oom_path_calls_failure_handler(void) {
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        obstack_exit_failure = 79;
        void* p = xmalloc((size_t)SIZE_MAX);
        if (p)
            free(p);
        _exit(1);
    }
    int code = wait_child_exit(pid);
    assert(code == 79);
}

int main(void) {
    test_default_alloc_failed_handler_exits_with_configured_code();
    test_xmalloc_failed_exits_with_configured_code();
    test_xmalloc_oom_path_calls_failure_handler();
    return 0;
}
