#include "obstack.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>  // Include for va_list, va_start, va_end
#include <errno.h>

// Ensure obstack functions are declared
int obstack_vprintf(struct obstack* obs, const char* format, va_list args);
size_t obstack_calculate_object_size(struct obstack* ob);

// Implementation for obstack_calculate_object_size
size_t obstack_calculate_object_size(struct obstack* ob) {
    return ob->next_free - ob->object_base;
}

// Implementation for obstack_printf
int obstack_printf(struct obstack* obs, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = obstack_vprintf(obs, format, args);
    va_end(args);
    return result;
}

void test_obstack_begin() {
    struct obstack obs;
    int result = obstack_init(&obs);  // Use public API for initialization
    assert(result == 1);
    assert(obs.chunk != NULL);
    obstack_free(&obs, NULL);  // Clean up
    printf("obstack_begin passed\n");
}

void test_obstack_newchunk() {
    struct obstack obs;
    obstack_init(&obs);

    void* initial_base = obs.object_base;
    _obstack_newchunk(&obs, 1024);  // Using the public function
    assert(obs.object_base != initial_base);
    obstack_free(&obs, NULL);  // Clean up
    printf("obstack_newchunk passed\n");
}

void test_obstack_free() {
    struct obstack obs;
    obstack_init(&obs);

    int* data = (int*)obstack_alloc(&obs, sizeof(int));
    *data = 42;

    obstack_free(&obs, data);   // Free the object
    assert(obs.chunk != NULL);  // The chunk should still exist after freeing the object
    printf("obstack_free passed\n");
}

void test_obstack_printf() {
    struct obstack obs;
    obstack_init(&obs);

    int result = obstack_printf(&obs, "Hello, %s!", "world");
    assert(result > 0);

    size_t size = obstack_calculate_object_size(&obs);
    assert(size > 0);
    obstack_free(&obs, NULL);  // Clean up
    printf("obstack_printf passed\n");
}

void test_obstack_memory_used() {
    struct obstack obs;
    obstack_init(&obs);

    int* data1 = (int*)obstack_alloc(&obs, 128);
    *data1 = 1;  // Use the allocated memory
    int* data2 = (int*)obstack_alloc(&obs, 256);
    *data2 = 2;  // Use the allocated memory

    _OBSTACK_SIZE_T memory_used = obstack_memory_used(&obs);
    assert(memory_used >= 128 + 256);
    obstack_free(&obs, NULL);  // Clean up
    printf("obstack_memory_used passed\n");
}

int main() {
    test_obstack_begin();
    test_obstack_newchunk();
    test_obstack_free();
    test_obstack_printf();
    test_obstack_memory_used();

    printf("All tests passed!\n");
    return 0;
}
