#include <obstack.h>

#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void* __real_malloc(size_t size);

static int fail_malloc_once;
static jmp_buf reentrant_jmp;
static int primary_handler_calls;
static int fallback_handler_calls;
static void (*saved_reentrant_handler)(void);

void* __wrap_malloc(size_t size) {
    if (fail_malloc_once) {
        fail_malloc_once = 0;
        errno = ENOMEM;
        return NULL;
    }
    return __real_malloc(size);
}

static int wait_child_exit(pid_t pid) {
    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    return WEXITSTATUS(status);
}

static void* always_fail_alloc(size_t size) {
    (void)size;
    errno = ENOMEM;
    return NULL;
}

static void never_called_free(void* p) {
    (void)p;
}

static void fallback_longjmp_handler(void) {
    fallback_handler_calls++;
    longjmp(reentrant_jmp, 2);
}

static void restore_then_reenter_handler(void) {
    primary_handler_calls++;
    obstack_alloc_failed_handler = saved_reentrant_handler;

    struct obstack ob;
    (void)_obstack_begin(&ob, 64, 0, always_fail_alloc, never_called_free);
    assert(!"nested _obstack_begin must not return");
}

static void test_alloc_failed_handler_restore_and_reentrancy(void) {
    void (*saved)(void) = obstack_alloc_failed_handler;

    primary_handler_calls = 0;
    fallback_handler_calls = 0;
    saved_reentrant_handler = fallback_longjmp_handler;
    obstack_alloc_failed_handler = restore_then_reenter_handler;

    int jumped = setjmp(reentrant_jmp);
    if (jumped == 0) {
        struct obstack ob;
        (void)_obstack_begin(&ob, 64, 0, always_fail_alloc, never_called_free);
        assert(!"expected _obstack_begin failure");
    }

    assert(jumped == 2);
    assert(primary_handler_calls == 1);
    assert(fallback_handler_calls == 1);
    assert(obstack_alloc_failed_handler == saved_reentrant_handler);

    obstack_alloc_failed_handler = saved;
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
        fail_malloc_once = 1;
        void* p = xmalloc((size_t)4096);
        if (p)
            free(p);
        _exit(1);
    }
    int code = wait_child_exit(pid);
    assert(code == 79);
}

int main(void) {
    test_alloc_failed_handler_restore_and_reentrancy();
    test_default_alloc_failed_handler_exits_with_configured_code();
    test_xmalloc_failed_exits_with_configured_code();
    test_xmalloc_oom_path_calls_failure_handler();
    return 0;
}
