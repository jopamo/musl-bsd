#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    const char* t = "hello";
    obstack_grow(&ob, t, 5);
    size_t sz = obstack_calculate_object_size(&ob);
    assert(sz == 5);

    obstack_1grow(&ob, '\0');
    (void)obstack_finish(&ob);
    assert(obstack_calculate_object_size(&ob) == 0);

    obstack_free(&ob, NULL);
    puts("test_calculate_object_size ok");
    return 0;
}
