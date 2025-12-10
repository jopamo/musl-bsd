#include "obstack_test_common.h"

#include <stdlib.h>

void* obstack_plain_alloc(size_t n) {
    return malloc(n ? n : 1);
}

void obstack_plain_free(void* p) {
    free(p);
}

void* obstack_extra_alloc(void* arg, size_t n) {
    struct extra_state* s = (struct extra_state*)arg;
    if (n == 0)
        n = 1;
    if (s->fail_after_bytes && s->total + n > s->fail_after_bytes)
        return NULL;
    void* p = malloc(n);
    if (p) {
        s->total += n;
        s->calls++;
    }
    return p;
}

void obstack_extra_free(void* arg, void* p) {
    (void)arg;
    free(p);
}

char* obstack_build_string(struct obstack* ob, size_t n, char fill) {
    if (n) {
        size_t chunk = 41;
        while (n >= chunk) {
            char tmp[64];
            size_t use = chunk < sizeof(tmp) ? chunk : sizeof(tmp);
            for (size_t i = 0; i < use; i++)
                tmp[i] = fill;
            obstack_grow(ob, tmp, use);
            n -= use;
        }
        while (n--)
            obstack_1grow(ob, fill);
    }
    obstack_1grow(ob, '\0');
    return (char*)obstack_finish(ob);
}
