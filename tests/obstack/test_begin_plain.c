#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);
    assert(ob.chunk != NULL);
    assert(ob.object_base != NULL);
    obstack_free(&ob, NULL);
    puts("test_begin_plain ok");
    return 0;
}
