#include "obstack_test_common.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

int __real_vsnprintf(char* str, size_t size, const char* format, va_list ap);

enum fail_stage {
    FAIL_NONE = 0,
    FAIL_MEASURE,
    FAIL_WRITE,
};

static enum fail_stage active_fail_stage;
static int wrapped_calls;

int __wrap_vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    wrapped_calls++;

    if ((active_fail_stage == FAIL_MEASURE && wrapped_calls == 1) ||
        (active_fail_stage == FAIL_WRITE && wrapped_calls == 2)) {
        errno = EIO;
        return -1;
    }

    return __real_vsnprintf(str, size, format, ap);
}

static int call_obstack_vprintf(struct obstack* ob, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rc = obstack_vprintf(ob, fmt, ap);
    va_end(ap);
    return rc;
}

static void test_measure_failure_does_not_advance_state(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 128, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    char* before = ob.next_free;
    active_fail_stage = FAIL_MEASURE;
    wrapped_calls = 0;

    int rc = call_obstack_vprintf(&ob, "x=%d", 42);
    assert(rc == -1);
    assert(wrapped_calls == 1);
    assert(ob.next_free == before);
    assert(ob.object_base == before);

    active_fail_stage = FAIL_NONE;
    _obstack_free(&ob, NULL);
}

static void test_write_failure_does_not_advance_state(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 128, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    char* before = ob.next_free;
    active_fail_stage = FAIL_WRITE;
    wrapped_calls = 0;

    int rc = call_obstack_vprintf(&ob, "tag:%s", "abc");
    assert(rc == -1);
    assert(wrapped_calls == 2);
    assert(ob.next_free == before);
    assert(ob.object_base == before);

    active_fail_stage = FAIL_NONE;
    _obstack_free(&ob, NULL);
}

int main(void) {
    test_measure_failure_does_not_advance_state();
    test_write_failure_does_not_advance_state();
    puts("test_vprintf_failures ok");
    return 0;
}
