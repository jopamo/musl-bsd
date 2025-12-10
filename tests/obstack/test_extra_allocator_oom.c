#include "obstack_test_common.h"

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

static jmp_buf g_oom_jmp;
static void oom_handler_longjmp(void) {
    longjmp(g_oom_jmp, 1);
}

extern void (*obstack_alloc_failed_handler)(void);

int main(void) {
    struct obstack ob;
    struct extra_state st = {0};
    st.fail_after_bytes = 512;

    int ok = _obstack_begin_1(&ob, 256, 0, obstack_extra_alloc, obstack_extra_free, &st);
    assert(ok == 1);

    void (*saved)(void) = obstack_alloc_failed_handler;
    obstack_alloc_failed_handler = oom_handler_longjmp;

    int jumped = 0;
    if (setjmp(g_oom_jmp) == 0) {
        (void)obstack_build_string(&ob, 3000, 'q');
        assert(!"expected OOM path did not trigger");
    }
    else {
        jumped = 1;
    }

    obstack_alloc_failed_handler = saved;
    obstack_free(&ob, NULL);

    assert(jumped == 1);
    assert(st.calls >= 1);

    puts("test_extra_allocator_oom ok");
    return 0;
}
