#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 128, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);
    _OBSTACK_SIZE_T before = _obstack_memory_used(&ob);
    assert(before >= 128);

    (void)obstack_build_string(&ob, 400, 'm');
    _OBSTACK_SIZE_T mid = _obstack_memory_used(&ob);
    assert(mid >= before);

    obstack_free(&ob, NULL);
    _OBSTACK_SIZE_T after = _obstack_memory_used(&ob);
    assert(after > 0);
    assert(after <= mid);
    assert(ob.chunk != NULL);
    assert(ob.chunk->prev == NULL);

    puts("test_memory_used ok");
    return 0;
}
