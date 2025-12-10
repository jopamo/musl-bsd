#ifndef OBSTACK_TEST_COMMON_H
#define OBSTACK_TEST_COMMON_H

#include "obstack.h"

#include <stdint.h>
#include <stddef.h>

struct extra_state {
    size_t total;
    int calls;
    size_t fail_after_bytes; /* 0 means unlimited */
};

void* obstack_plain_alloc(size_t n);
void obstack_plain_free(void* p);

void* obstack_extra_alloc(void* arg, size_t n);
void obstack_extra_free(void* arg, void* p);

char* obstack_build_string(struct obstack* ob, size_t n, char fill);
void obstack_test_destroy(struct obstack* ob);

#endif /* OBSTACK_TEST_COMMON_H */
