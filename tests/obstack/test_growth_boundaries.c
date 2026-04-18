#include "obstack_test_common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct alloc_trace {
    size_t requests[8];
    int alloc_calls;
    int free_calls;
};

static void* trace_alloc(void* arg, size_t n) {
    struct alloc_trace* trace = (struct alloc_trace*)arg;
    assert((size_t)trace->alloc_calls < (sizeof(trace->requests) / sizeof(trace->requests[0])));
    trace->requests[trace->alloc_calls++] = n;
    if (n == 0)
        n = 1;
    return malloc(n);
}

static void trace_free(void* arg, void* p) {
    struct alloc_trace* trace = (struct alloc_trace*)arg;
    trace->free_calls++;
    free(p);
}

static void test_newchunk_respects_chunk_size_floor(void) {
    struct alloc_trace trace = {0};
    struct obstack ob;
    int ok = _obstack_begin_1(&ob, 256, 16, trace_alloc, trace_free, &trace);
    assert(ok == 1);
    assert(trace.alloc_calls == 1);
    assert(trace.requests[0] == 256);

    _obstack_newchunk(&ob, 1);
    assert(trace.alloc_calls == 2);
    assert(trace.requests[1] == 256);

    _obstack_free(&ob, NULL);
    assert(trace.free_calls == 2);
}

static void test_finish_alignment_rollover_clamps_to_chunk_limit(void) {
    struct alloc_trace trace = {0};
    struct obstack ob;
    int ok = _obstack_begin_1(&ob, 128, 32, trace_alloc, trace_free, &trace);
    assert(ok == 1);

    _OBSTACK_SIZE_T room = obstack_room(&ob);
    assert(room > 1);
    obstack_blank(&ob, room - 1);
    assert(ob.next_free == ob.chunk_limit - 1);

    char* start = ob.object_base;
    void* obj = obstack_finish(&ob);
    assert(obj == start);
    assert(ob.object_base == ob.chunk_limit);
    assert(ob.next_free == ob.chunk_limit);

    obstack_1grow(&ob, 'Q');
    assert(trace.alloc_calls == 2);
    char* one = (char*)obstack_finish(&ob);
    assert(one[0] == 'Q');

    _obstack_free(&ob, NULL);
    assert(trace.free_calls == 2);
}

static void test_object_size_tracking_across_growth_boundaries(void) {
    struct obstack ob;
    int ok = _obstack_begin(&ob, 96, 0, obstack_plain_alloc, obstack_plain_free);
    assert(ok == 1);
    assert(obstack_calculate_object_size(&ob) == 0);

    unsigned char a[17];
    memset(a, 'A', sizeof(a));
    obstack_grow(&ob, a, sizeof(a));
    size_t expected = sizeof(a);
    assert(obstack_calculate_object_size(&ob) == expected);

    unsigned char b[200];
    memset(b, 'B', sizeof(b));
    obstack_grow(&ob, b, sizeof(b));
    expected += sizeof(b);
    assert(obstack_calculate_object_size(&ob) == expected);

    obstack_blank(&ob, 9);
    expected += 9;
    assert(obstack_calculate_object_size(&ob) == expected);

    unsigned char* out = (unsigned char*)obstack_finish(&ob);
    assert(memcmp(out, a, sizeof(a)) == 0);
    assert(memcmp(out + sizeof(a), b, sizeof(b)) == 0);
    assert(obstack_calculate_object_size(&ob) == 0);

    obstack_grow(&ob, "xy", 2);
    assert(obstack_calculate_object_size(&ob) == 2);

    _obstack_free(&ob, NULL);
}

int main(void) {
    test_newchunk_respects_chunk_size_floor();
    test_finish_alignment_rollover_clamps_to_chunk_limit();
    test_object_size_tracking_across_growth_boundaries();
    puts("test_growth_boundaries ok");
    return 0;
}
