// obstack_tests.c
// comprehensive tests for your obstack implementation
// single-line comments have no trailing periods

#include "obstack.h"
#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* plain_alloc(size_t n) {
    return malloc(n ? n : 1);
}
static void plain_free(void* p) {
    free(p);
}

struct extra_state {
    size_t total;
    int calls;
    size_t fail_after_bytes; /* 0 means unlimited */
};

static void* extra_alloc(void* arg, size_t n) {
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

static void extra_free(void* arg, void* p) {
    (void)arg;
    free(p);
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static void fill_pattern(char* p, size_t n, unsigned seed) {
    for (size_t i = 0; i < n; i++)
        p[i] = (char)('A' + ((i + seed) % 26));
}

static char* build_string(struct obstack* ob, size_t n, char fill) {
    if (n) {
        size_t chunk = 41;
        while (n >= chunk) {
            char tmp[64];
            size_t use = chunk < sizeof(tmp) ? chunk : sizeof(tmp);
            memset(tmp, (unsigned char)fill, use);
            obstack_grow(ob, tmp, use);
            n -= use;
        }
        while (n--)
            obstack_1grow(ob, fill);
    }
    obstack_1grow(ob, '\0');
    return (char*)obstack_finish(ob);
}

static void test_begin_plain(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 0, plain_alloc, plain_free);
    assert(ok == 1);
    assert(ob.chunk != NULL);
    assert(ob.object_base != NULL);
    obstack_free(&ob, NULL);
    puts("test_begin_plain ok");
}

static void test_begin_extra(void) {
    struct obstack ob;
    struct extra_state st = {0};
    int ok = _obstack_begin_1(&ob, 0, 0, extra_alloc, extra_free, &st);
    assert(ok == 1);
    assert(st.calls >= 1);
    char* s = build_string(&ob, 32, 'x');
    assert(strcmp(s, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx") == 0);
    obstack_free(&ob, NULL);
    puts("test_begin_extra ok");
}

static void test_force_newchunk(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 128, sizeof(void*), plain_alloc, plain_free);
    assert(ok == 1);

    void* base0 = ob.object_base;
    size_t used0 = (size_t)(ob.next_free - ob.object_base);

    char* a = build_string(&ob, 300, 'a');
    assert(strlen(a) == 300);
    assert(a[299] == 'a');

    _OBSTACK_SIZE_T bytes = _obstack_memory_used(&ob);
    assert(bytes >= 128);

    obstack_grow(&ob, "zzz", 3);
    _obstack_newchunk(&ob, 1024);
    assert(ob.object_base != base0 || (size_t)(ob.next_free - ob.object_base) != used0);

    obstack_free(&ob, NULL);
    puts("test_force_newchunk ok");
}

static void test_free_to_object(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 256, 0, plain_alloc, plain_free);
    assert(ok == 1);

    char* a = build_string(&ob, 50, 'a');
    char* b = build_string(&ob, 200, 'b');
    char* c = build_string(&ob, 10, 'c');
    (void)a;
    (void)c;

    obstack_free(&ob, b);
    assert(ob.object_base == b);

    obstack_grow(&ob, "TAG", 4);
    char* b2 = (char*)obstack_finish(&ob);
    assert(strcmp(b2, "TAG") == 0);

    obstack_free(&ob, NULL);
    puts("test_free_to_object ok");
}

static void test_alignment(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 32, plain_alloc, plain_free);
    assert(ok == 1);
    uintptr_t base = (uintptr_t)ob.object_base;
    assert((base % 32) == 0);

    obstack_grow(&ob, "abc", 3);
    obstack_1grow(&ob, '\0');
    char* s = (char*)obstack_finish(&ob);
    assert(strcmp(s, "abc") == 0);

    obstack_free(&ob, NULL);
    puts("test_alignment ok");
}

static void test_memory_used(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 128, 0, plain_alloc, plain_free);
    assert(ok == 1);
    _OBSTACK_SIZE_T before = _obstack_memory_used(&ob);
    assert(before >= 128);

    (void)build_string(&ob, 400, 'm');
    _OBSTACK_SIZE_T mid = _obstack_memory_used(&ob);
    assert(mid >= before);

    obstack_free(&ob, NULL);
    _OBSTACK_SIZE_T after = _obstack_memory_used(&ob);
    assert(after == 0);

    puts("test_memory_used ok");
}

static void test_printf_small(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 0, plain_alloc, plain_free);
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
}

static void test_printf_len_guard(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 0, plain_alloc, plain_free);
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
}

/* catch OOM by swapping the global handler to a longjmp stub */
static jmp_buf g_oom_jmp;
static void oom_handler_longjmp(void) {
    longjmp(g_oom_jmp, 1);
}
/* defined in your obstack.c */
extern void (*obstack_alloc_failed_handler)(void);

static void test_extra_allocator_oom(void) {
    struct obstack ob;
    struct extra_state st = {0};
    st.fail_after_bytes = 512;

    int ok = _obstack_begin_1(&ob, 256, 0, extra_alloc, extra_free, &st);
    assert(ok == 1);

    void (*saved)(void) = obstack_alloc_failed_handler;
    obstack_alloc_failed_handler = oom_handler_longjmp;

    int jumped = 0;
    if (setjmp(g_oom_jmp) == 0) {
        /* push far enough to require a new chunk that the capped allocator will refuse */
        (void)build_string(&ob, 3000, 'q');
        assert(0 && "expected OOM path did not trigger");
    }
    else {
        jumped = 1;
    }

    /* restore handler and cleanup no matter what */
    obstack_alloc_failed_handler = saved;
    obstack_free(&ob, NULL);

    assert(jumped == 1);
    assert(st.calls >= 1);

    puts("test_extra_allocator_oom ok");
}

static void test_calculate_object_size(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 0, 0, plain_alloc, plain_free);
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
}

int main(void) {
    test_begin_plain();
    test_begin_extra();
    test_force_newchunk();
    test_free_to_object();
    test_alignment();
    test_memory_used();
    test_printf_small();
    test_printf_len_guard();
    test_extra_allocator_oom();  // now intercepted safely
    test_calculate_object_size();
    puts("all obstack tests passed");
    return 0;
}
