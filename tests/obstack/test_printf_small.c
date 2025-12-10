#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    int len = obstack_printf(&ob, "num=%d hex=%x str=%s", 42, 0x2a, "ok");
    obstack_1grow(&ob, '\0');
    char* s = (char*)obstack_finish(&ob);
    assert(strcmp(s, "num=42 hex=2a str=ok") == 0);
    assert(len == (int)strlen(s));

    size_t cur = obstack_calculate_object_size(&ob);
    assert(cur == 0);

    obstack_free(&ob, NULL);
    puts("test_printf_small ok");
    return 0;
}
