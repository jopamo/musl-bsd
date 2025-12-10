#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 128, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    struct _obstack_chunk* first = ob.chunk;
    (void)obstack_build_string(&ob, 32, 'a');

    obstack_free(&ob, NULL);
    assert(ob.chunk == NULL);
    assert(ob.object_base == NULL);
    assert(ob.next_free == NULL);

    ok = _obstack_begin(&ob, 128, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);
    assert(ob.chunk != first);

    char* b = obstack_build_string(&ob, 8, 'b');
    assert(strcmp(b, "bbbbbbbb") == 0);

    obstack_free(&ob, NULL);
    puts("test_free_reset ok");
    return 0;
}
