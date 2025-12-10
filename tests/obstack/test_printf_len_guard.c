#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    char big[800];
    memset(big, 'X', sizeof(big));
    big[sizeof(big) - 1] = '\0';

    int len = obstack_printf(&ob, "prefix:%s", big);
    obstack_1grow(&ob, '\0');
    char* out = (char*)obstack_finish(&ob);

    assert(len == (int)strlen(out));
    assert(strncmp(out, "prefix:", 7) == 0);
    assert(strlen(out) == 7 + strlen(big));

    obstack_free(&ob, NULL);
    puts("test_printf_len_guard ok");
    return 0;
}
