#include "obstack_test_common.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 32, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);
    uintptr_t base = (uintptr_t)ob.object_base;
    assert((base % 32) == 0);

    obstack_grow(&ob, "abc", 3);
    obstack_1grow(&ob, '\0');
    char* s = (char*)obstack_finish(&ob);
    assert(strcmp(s, "abc") == 0);

    obstack_free(&ob, NULL);
    puts("test_alignment ok");
    return 0;
}
