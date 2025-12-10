#include "obstack.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define _OBSTACK_NORETURN _Noreturn
#else
#define _OBSTACK_NORETURN __attribute__((__noreturn__))
#endif

#ifndef RESULT_TYPE
#define RESULT_TYPE int
#define OBSTACK_PRINTF obstack_printf
#define OBSTACK_VPRINTF obstack_vprintf
#endif

int obstack_exit_failure = EXIT_FAILURE;

static _OBSTACK_NORETURN void obstack_default_alloc_failed(void) {
    fprintf(stderr, "obstack: memory exhausted\n");
    exit(obstack_exit_failure);
}

void (*obstack_alloc_failed_handler)(void) = obstack_default_alloc_failed;

#ifndef __BPTR_ALIGN
#define __BPTR_ALIGN(B, P, A) ((B) + ((((P) - (B)) + (A)) & ~(A)))
#endif

#ifndef __PTR_ALIGN
#define __PTR_ALIGN(B, P, A) \
    (sizeof(ptrdiff_t) < sizeof(void*) ? __BPTR_ALIGN(B, P, A) : (char*)(((ptrdiff_t)(P) + (A)) & ~(A)))
#endif

void xmalloc_failed(size_t size) {
    fprintf(stderr, "\nout of memory allocating %lu bytes\n", (unsigned long)size);
    exit(obstack_exit_failure);
}

void* xmalloc(size_t size) {
    if (size == 0) {
        size = 1;
    }

    void* newmem = malloc(size);
    if (!newmem) {
        xmalloc_failed(size);
    }
    return newmem;
}

static void* call_chunkfun(struct obstack* h, size_t size) {
    if (h->use_extra_arg) {
        return h->chunkfun.extra(h->extra_arg, size);
    }
    return h->chunkfun.plain(size);
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
    if (alignment == 0) {
        alignment = DEFAULT_ALIGNMENT;
    }

    if (size == 0) {
        _OBSTACK_SIZE_T extra = ((((12 + DEFAULT_ROUNDING - 1) & ~(DEFAULT_ROUNDING - 1)) + 4 + DEFAULT_ROUNDING - 1) &
                                 ~(DEFAULT_ROUNDING - 1));
        size = 4096 - extra;
    }

    h->chunk_size = size;
    h->alignment_mask = alignment - 1;

    struct _obstack_chunk* chunk = (struct _obstack_chunk*)call_chunkfun(h, h->chunk_size);
    if (!chunk) {
        (*obstack_alloc_failed_handler)();
    }

    chunk->prev = NULL;
    chunk->limit = (char*)chunk + h->chunk_size;

    h->chunk = chunk;
    h->chunk_limit = chunk->limit;

    char* aligned = __PTR_ALIGN((char*)chunk, chunk->contents, h->alignment_mask);
    h->object_base = h->next_free = aligned;
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
    size_t obj_size = (size_t)(h->next_free - h->object_base);

    size_t new_size = obj_size + (size_t)length;
    if (new_size < obj_size) {
        (*obstack_alloc_failed_handler)();
    }

    if (SIZE_MAX - new_size < h->alignment_mask + 100) {
        (*obstack_alloc_failed_handler)();
    }
    new_size += h->alignment_mask + 100;

    size_t growth = obj_size >> 3;
    if (SIZE_MAX - new_size < growth) {
        (*obstack_alloc_failed_handler)();
    }
    new_size += growth;

    if (new_size < h->chunk_size) {
        new_size = h->chunk_size;
    }

    struct _obstack_chunk* new_chunk = (struct _obstack_chunk*)call_chunkfun(h, new_size);
    if (!new_chunk) {
        (*obstack_alloc_failed_handler)();
    }
    new_chunk->prev = old_chunk;
    new_chunk->limit = (char*)new_chunk + new_size;
    h->chunk_limit = new_chunk->limit;

    char* old_base = h->object_base;
    char* new_base = __PTR_ALIGN((char*)new_chunk, new_chunk->contents, h->alignment_mask);
    memcpy(new_base, old_base, obj_size);
    h->object_base = new_base;
    h->next_free = new_base + obj_size;

    if (!h->maybe_empty_object) {
        char* old_aligned = __PTR_ALIGN((char*)old_chunk, old_chunk->contents, h->alignment_mask);
        if (old_base == old_aligned) {
            new_chunk->prev = old_chunk->prev;
            call_freefun(h, old_chunk);
        }
    }

    h->chunk = new_chunk;
    h->maybe_empty_object = 0;
    h->alloc_failed = 0;
}

void _obstack_free(struct obstack* h, void* obj) {
    struct _obstack_chunk* chunk = h->chunk;
    if (!chunk) {
        return;
    }

    if (obj == NULL) {
        while (chunk->prev) {
            struct _obstack_chunk* prev = chunk->prev;
            call_freefun(h, chunk);
            chunk = prev;
        }
        h->chunk = chunk;
        h->chunk_limit = chunk->limit;
        h->object_base = h->next_free = __PTR_ALIGN((char*)chunk, chunk->contents, h->alignment_mask);
        h->maybe_empty_object = 0;
        h->alloc_failed = 0;
        return;
    }

    while (chunk && !((char*)obj >= (char*)chunk && (char*)obj < (char*)chunk->limit)) {
        struct _obstack_chunk* prev = chunk->prev;
        call_freefun(h, chunk);
        chunk = prev;
        h->maybe_empty_object = 1;
    }

    if (!chunk) {
        abort();
    }

    h->chunk = chunk;
    h->chunk_limit = chunk->limit;
    h->object_base = h->next_free = (char*)obj;
    h->maybe_empty_object = (h->object_base == h->next_free);
    h->alloc_failed = 0;
}

_OBSTACK_SIZE_T _obstack_memory_used(struct obstack* h) {
    _OBSTACK_SIZE_T total = 0;
    for (struct _obstack_chunk* chunk = h->chunk; chunk; chunk = chunk->prev) {
        total += (_OBSTACK_SIZE_T)((char*)chunk->limit - (char*)chunk);
    }
    return total;
}

size_t obstack_calculate_object_size(struct obstack* ob) {
    return (size_t)(ob->next_free - ob->object_base);
}

RESULT_TYPE OBSTACK_VPRINTF(struct obstack* obstack, const char* __restrict fmt, va_list ap) {
    va_list measure;
    va_copy(measure, ap);
    int needed = vsnprintf(NULL, 0, fmt, measure);
    va_end(measure);
    if (needed < 0) {
        return needed;
    }

    size_t total = (size_t)needed + 1;
    if (obstack_room(obstack) < total) {
        _obstack_newchunk(obstack, total);
    }

    char* dest = obstack->next_free;
    va_list copy;
    va_copy(copy, ap);
    int written = vsnprintf(dest, total, fmt, copy);
    va_end(copy);
    if (written < 0) {
        return written;
    }

    if ((size_t)written >= total) {
        size_t required = (size_t)written + 1;
        if (obstack_room(obstack) < required) {
            _obstack_newchunk(obstack, required);
        }
        dest = obstack->next_free;
        va_copy(copy, ap);
        written = vsnprintf(dest, required, fmt, copy);
        va_end(copy);
        if (written < 0) {
            return written;
        }
    }

    obstack->next_free = dest + (size_t)written;
    return written;
}

RESULT_TYPE OBSTACK_PRINTF(struct obstack* obstack, const char* __restrict fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    RESULT_TYPE res = OBSTACK_VPRINTF(obstack, fmt, ap);
    va_end(ap);
    return res;
}
