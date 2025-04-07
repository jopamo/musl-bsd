#include "obstack.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static _Noreturn void obstack_default_alloc_failed(void) {
    fprintf(stderr, "obstack: memory exhausted\n");
    exit(EXIT_FAILURE);
}

void (*obstack_alloc_failed_handler)(void) = obstack_default_alloc_failed;

#ifndef __BPTR_ALIGN
#define __BPTR_ALIGN(B, P, A) ((B) + ((((P) - (B)) + (A)) & ~(A)))
#endif
#ifndef __PTR_ALIGN
#define __PTR_ALIGN(B, P, A) \
    (sizeof(ptrdiff_t) < sizeof(void*) ? __BPTR_ALIGN(B, P, A) : (char*)(((ptrdiff_t)(P) + (A)) & ~(A)))
#endif

static void* call_chunkfun(struct obstack* h, size_t size) {
    if (h->use_extra_arg) {
        return h->chunkfun.extra(h->extra_arg, size);
    }
    else {
        return h->chunkfun.plain(size);
    }
}

static void call_freefun(struct obstack* h, void* chunk) {
    if (h->use_extra_arg) {
        h->freefun.extra(h->extra_arg, chunk);
    }
    else {
        h->freefun.plain(chunk);
    }
}

static int _obstack_begin_worker(struct obstack* h, _OBSTACK_SIZE_T size, _OBSTACK_SIZE_T alignment) {
    struct _obstack_chunk* chunk = NULL;

    if (alignment == 0) {
        alignment = DEFAULT_ALIGNMENT;
    }

    if (size == 0) {
        int extra = ((((12 + DEFAULT_ROUNDING - 1) & ~(DEFAULT_ROUNDING - 1)) + 4 + DEFAULT_ROUNDING - 1) &
                     ~(DEFAULT_ROUNDING - 1));
        size = 8192 - extra;
    }

    h->chunk_size = size;
    h->alignment_mask = alignment - 1;

    chunk = (struct _obstack_chunk*)call_chunkfun(h, h->chunk_size);
    if (!chunk) {
        (*obstack_alloc_failed_handler)();
    }

    h->chunk = chunk;
    chunk->prev = NULL;
    chunk->limit = (char*)chunk + h->chunk_size;

    {
        char* aligned = __PTR_ALIGN((char*)chunk, chunk->contents, h->alignment_mask);
        h->object_base = h->next_free = aligned;
    }
    h->chunk_limit = chunk->limit;
    h->maybe_empty_object = 0;
    h->alloc_failed = 0;
    return 1;
}

int _obstack_begin(struct obstack* h,
                   _OBSTACK_SIZE_T size,
                   _OBSTACK_SIZE_T alignment,
                   void* (*chunkfun)(size_t),
                   void (*freefun)(void*)) {
    h->chunkfun.plain = chunkfun;
    h->freefun.plain = freefun;
    h->use_extra_arg = 0;

    return _obstack_begin_worker(h, size, alignment);
}

int _obstack_begin_1(struct obstack* h,
                     _OBSTACK_SIZE_T size,
                     _OBSTACK_SIZE_T alignment,
                     void* (*chunkfun)(void*, size_t),
                     void (*freefun)(void*, void*),
                     void* arg) {
    h->chunkfun.extra = chunkfun;
    h->freefun.extra = freefun;
    h->extra_arg = arg;
    h->use_extra_arg = 1;

    return _obstack_begin_worker(h, size, alignment);
}

void _obstack_newchunk(struct obstack* h, _OBSTACK_SIZE_T length) {
    struct _obstack_chunk* old_chunk = h->chunk;
    struct _obstack_chunk* new_chunk = NULL;
    size_t obj_size = (size_t)(h->next_free - h->object_base);

    {
        size_t sum1 = obj_size + length;
        size_t sum2 = sum1 + h->alignment_mask;
        size_t new_size = sum2 + (obj_size >> 3) + 100;

        if (new_size < sum2) {
            new_size = sum2;
        }
        if (new_size < h->chunk_size) {
            new_size = h->chunk_size;
        }

        new_chunk = (struct _obstack_chunk*)call_chunkfun(h, new_size);
        if (!new_chunk) {
            (*obstack_alloc_failed_handler)();
        }
        new_chunk->prev = old_chunk;
        new_chunk->limit = (char*)new_chunk + new_size;
        h->chunk_limit = new_chunk->limit;
    }

    {
        char* aligned = __PTR_ALIGN((char*)new_chunk, new_chunk->contents, h->alignment_mask);

        memcpy(aligned, h->object_base, obj_size);
        h->object_base = aligned;
        h->next_free = aligned + obj_size;
    }

    if (!h->maybe_empty_object) {
        char* old_aligned = __PTR_ALIGN((char*)old_chunk, old_chunk->contents, h->alignment_mask);
        if (h->object_base == old_aligned) {
            new_chunk->prev = old_chunk->prev;
            call_freefun(h, old_chunk);
        }
    }

    h->chunk = new_chunk;
    h->maybe_empty_object = 0;
}

void _obstack_free(struct obstack* h, void* obj) {
    struct _obstack_chunk* lp = h->chunk;

    if (obj == NULL) {
        while (lp) {
            struct _obstack_chunk* prev = lp->prev;
            call_freefun(h, lp);
            lp = prev;
        }
        h->chunk = NULL;
        return;
    }

    while (lp) {
        if ((obj >= (void*)lp) && (obj < (void*)lp->limit)) {
            h->chunk = lp;
            h->chunk_limit = lp->limit;
            h->object_base = h->next_free = (char*)obj;
            return;
        }
        {
            struct _obstack_chunk* prev = lp->prev;
            call_freefun(h, lp);
            lp = prev;
        }

        h->maybe_empty_object = 1;
    }

    abort();
}

_OBSTACK_SIZE_T _obstack_memory_used(struct obstack* h) {
    _OBSTACK_SIZE_T total = 0;
    struct _obstack_chunk* chunk = h->chunk;
    while (chunk) {
        total += (_OBSTACK_SIZE_T)((char*)chunk->limit - (char*)chunk);
        chunk = chunk->prev;
    }
    return total;
}

int obstack_printf(struct obstack* ob, const char* __restrict fmt, ...) {
    char buffer[1024];
    va_list ap;
    int length;

    va_start(ap, fmt);
    length = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    if (length >= (int)sizeof(buffer)) {
        fprintf(stderr, "Warning: obstack_printf truncated!\n");
        length = (int)(sizeof(buffer) - 1);
        buffer[length] = '\0';
    }

    obstack_grow(ob, buffer, length);
    return length;
}
