#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 256, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    char* a = obstack_build_string(&ob, 50, 'a');
    char* b = obstack_build_string(&ob, 200, 'b');
    char* c = obstack_build_string(&ob, 10, 'c');
    (void)a;
    (void)c;

    obstack_free(&ob, b);
    assert(ob.object_base == b);

    obstack_grow(&ob, "TAG", 4);
    char* b2 = (char*)obstack_finish(&ob);
    assert(strcmp(b2, "TAG") == 0);

    obstack_free(&ob, NULL);
    puts("test_free_to_object ok");
    return 0;
}
