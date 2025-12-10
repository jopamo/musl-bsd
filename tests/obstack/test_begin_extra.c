#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct obstack ob;
    struct extra_state st = {0};
    int ok = _obstack_begin_1(&ob, 0, 0, obstack_extra_alloc, obstack_extra_free, &st);
    assert(ok == 1);
    assert(st.calls >= 1);
    char* s = obstack_build_string(&ob, 32, 'x');
    assert(strcmp(s, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx") == 0);
    obstack_free(&ob, NULL);
    puts("test_begin_extra ok");
    return 0;
}
