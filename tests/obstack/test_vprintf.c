#include "obstack_test_common.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int call_obstack_vprintf(struct obstack* ob, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rc = obstack_vprintf(ob, fmt, ap);
    va_end(ap);
    return rc;
}

int main(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 64, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);

    char big[256];
    memset(big, 'Z', sizeof(big) - 1);
    big[sizeof(big) - 1] = '\0';

    int len1 = obstack_printf(&ob, "tag:%d", 123);
    int len2 = call_obstack_vprintf(&ob, "|%s|", big);
    assert(len1 > 0);
    assert(len2 == (int)(strlen(big) + 2));

    obstack_1grow(&ob, '\0');
    char* out = (char*)obstack_finish(&ob);
    size_t out_len = strlen(out);
    assert(out_len == (size_t)(len1 + len2));
    assert(strncmp(out, "tag:123|", 8) == 0);
    assert(out[out_len - 1] == '|');
    for (size_t i = 8; i + 1 < out_len; i++) {
        assert(out[i] == 'Z');
    }

    obstack_free(&ob, NULL);
    puts("test_vprintf ok");
    return 0;
}
