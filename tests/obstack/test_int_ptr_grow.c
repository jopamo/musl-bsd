#include "obstack_test_common.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { GUARD_BYTES = 32, GUARD_PATTERN = 0xA5 };

struct guard_alloc {
    unsigned char* base;
    size_t size;
    struct guard_alloc* next;
};

static struct guard_alloc* guard_allocs;

static void check_guards(void) {
    for (struct guard_alloc* a = guard_allocs; a; a = a->next) {
        for (size_t i = 0; i < GUARD_BYTES; i++) {
            assert(a->base[a->size + i] == (unsigned char)GUARD_PATTERN);
        }
    }
}

static void* guard_chunk_alloc(size_t n) {
    if (n == 0)
        n = 1;

    struct guard_alloc* a = (struct guard_alloc*)malloc(sizeof(*a));
    assert(a != NULL);

    a->base = (unsigned char*)malloc(n + GUARD_BYTES);
    assert(a->base != NULL);
    a->size = n;
    memset(a->base + n, GUARD_PATTERN, GUARD_BYTES);
    a->next = guard_allocs;
    guard_allocs = a;
    return a->base;
}

static void guard_chunk_free(void* p) {
    struct guard_alloc** it = &guard_allocs;
    while (*it && (*it)->base != (unsigned char*)p) {
        it = &(*it)->next;
    }
    assert(*it != NULL);

    struct guard_alloc* a = *it;
    for (size_t i = 0; i < GUARD_BYTES; i++) {
        assert(a->base[a->size + i] == (unsigned char)GUARD_PATTERN);
    }

    *it = a->next;
    free(a->base);
    free(a);
}

static void begin_guarded_obstack(struct obstack* ob) {
    assert(guard_allocs == NULL);
    int ok = _obstack_begin(ob, 128, 0, guard_chunk_alloc, guard_chunk_free);
    assert(ok == 1);
}

static void end_guarded_obstack(struct obstack* ob) {
    check_guards();
    obstack_free(ob, NULL);
    assert(guard_allocs == NULL);
}

static void test_int_grow_stores_at_object_base(void) {
    struct obstack ob;
    begin_guarded_obstack(&ob);

    assert(obstack_room(&ob) >= (_OBSTACK_SIZE_T)(2 * sizeof(int)));
    memset(ob.next_free, 0, 2 * sizeof(int));

    const int value = 0x13579BDF;
    obstack_int_grow(&ob, value);

    int* out = (int*)obstack_finish(&ob);
    assert(*out == value);

    end_guarded_obstack(&ob);
}

static void test_ptr_grow_stores_at_object_base(void) {
    struct obstack ob;
    begin_guarded_obstack(&ob);

    assert(obstack_room(&ob) >= (_OBSTACK_SIZE_T)(2 * sizeof(void*)));
    memset(ob.next_free, 0, 2 * sizeof(void*));

    uintptr_t marker = 0x1234;
    void* expected = &marker;
    obstack_ptr_grow(&ob, expected);

    void** out = (void**)obstack_finish(&ob);
    assert(*out == expected);

    end_guarded_obstack(&ob);
}

static void test_int_grow_does_not_overflow_at_chunk_limit(void) {
    struct obstack ob;
    begin_guarded_obstack(&ob);

    _OBSTACK_SIZE_T room = obstack_room(&ob);
    assert(room > (_OBSTACK_SIZE_T)sizeof(int));
    obstack_blank(&ob, room - (_OBSTACK_SIZE_T)sizeof(int));
    assert(obstack_room(&ob) == (_OBSTACK_SIZE_T)sizeof(int));

    obstack_int_grow(&ob, 0x2468ACE0);
    check_guards();

    end_guarded_obstack(&ob);
}

static void test_ptr_grow_does_not_overflow_at_chunk_limit(void) {
    struct obstack ob;
    begin_guarded_obstack(&ob);

    _OBSTACK_SIZE_T room = obstack_room(&ob);
    assert(room > (_OBSTACK_SIZE_T)sizeof(void*));
    obstack_blank(&ob, room - (_OBSTACK_SIZE_T)sizeof(void*));
    assert(obstack_room(&ob) == (_OBSTACK_SIZE_T)sizeof(void*));

    void* expected = &ob;
    obstack_ptr_grow(&ob, expected);
    check_guards();

    end_guarded_obstack(&ob);
}

int main(void) {
    test_int_grow_stores_at_object_base();
    test_ptr_grow_stores_at_object_base();
    test_int_grow_does_not_overflow_at_chunk_limit();
    test_ptr_grow_does_not_overflow_at_chunk_limit();
    puts("test_int_ptr_grow ok");
    return 0;
}
