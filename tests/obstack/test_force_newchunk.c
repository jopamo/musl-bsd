#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 128, sizeof(void*), obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    void* base0 = ob.object_base;
    size_t used0 = (size_t)(ob.next_free - ob.object_base);

    char* a = obstack_build_string(&ob, 300, 'a');
    assert(a[299] == 'a');

    _OBSTACK_SIZE_T bytes = _obstack_memory_used(&ob);
    assert(bytes >= 128);

    obstack_grow(&ob, "zzz", 3);
    _obstack_newchunk(&ob, 1024);
    assert(ob.object_base != base0 || (size_t)(ob.next_free - ob.object_base) != used0);

    obstack_free(&ob, NULL);
    puts("test_force_newchunk ok");
    return 0;
}
