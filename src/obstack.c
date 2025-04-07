/*
   obstack.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h> /* for size_t, offsetof */
#include "obstack.h"

/* Provide fallback for _Noreturn if needed. */
#if !(defined _Noreturn || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112))
#if ((defined __GNUC__ && (__GNUC__ >= 3)) || (defined __SUNPRO_C && __SUNPRO_C >= 0x5110))
#define _Noreturn __attribute__((__noreturn__))
#elif defined _MSC_VER && _MSC_VER >= 1200
#define _Noreturn __declspec(noreturn)
#else
#define _Noreturn
#endif
#endif

/* Provide default definition of MAX if not present. */
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/*
 * Default alignment logic. This used to rely on glibc's internal
 * code. We replicate a common approach below.
 */
#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT MAX(_Alignof(long double), MAX(_Alignof(uintmax_t), _Alignof(void*)))
#endif

#ifndef DEFAULT_ROUNDING
#define DEFAULT_ROUNDING MAX(sizeof(long double), MAX(sizeof(uintmax_t), sizeof(void*)))
#endif

/*
 * call_chunkfun/call_freefun -- Indirection so obstack can optionally
 * use a chunk alloc/free function that has an extra `void*` argument.
 */
static void* call_chunkfun(struct obstack* h, size_t size) {
    if (h->use_extra_arg)
        return h->chunkfun.extra(h->extra_arg, size);
    else
        return h->chunkfun.plain(size);
}

static void call_freefun(struct obstack* h, void* old_chunk) {
    if (h->use_extra_arg)
        h->freefun.extra(h->extra_arg, old_chunk);
    else
        h->freefun.plain(old_chunk);
}

/*
 * _obstack_begin_worker -- Internal helper that sets up the first chunk.
 */
static int _obstack_begin_worker(struct obstack* h, _OBSTACK_SIZE_T size, _OBSTACK_SIZE_T alignment) {
    struct _obstack_chunk* chunk;

    if (alignment == 0)
        alignment = DEFAULT_ALIGNMENT;
    if (size == 0) {
        /* A 4096-based default chunk size, minus overhead. */
        int extra = ((((12 + DEFAULT_ROUNDING - 1) & ~(DEFAULT_ROUNDING - 1)) + 4 + DEFAULT_ROUNDING - 1) &
                     ~(DEFAULT_ROUNDING - 1));
        size = 4096 - extra;
    }

    h->chunk_size = size;
    h->alignment_mask = alignment - 1;

    chunk = (struct _obstack_chunk*)call_chunkfun(h, h->chunk_size);
    if (!chunk)
        (*obstack_alloc_failed_handler)();

    h->chunk = chunk;
    /* Align the object base pointer. */
    h->next_free = h->object_base = (char*)__PTR_ALIGN((char*)chunk, chunk->contents, alignment - 1);

    h->chunk_limit = chunk->limit = (char*)chunk + h->chunk_size;
    chunk->prev = NULL;
    h->maybe_empty_object = 0;
    h->alloc_failed = 0;
    return 1;
}

/*
 * _obstack_begin -- Public entry point that uses non-extra alloc functions.
 */
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

/*
 * _obstack_begin_1 -- Public entry point that uses extra-arg alloc functions.
 */
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

/*
 * _obstack_newchunk -- allocate a new chunk when we run out of room.
 */
void _obstack_newchunk(struct obstack* h, _OBSTACK_SIZE_T length) {
    struct _obstack_chunk* old_chunk = h->chunk;
    struct _obstack_chunk* new_chunk = NULL;
    size_t obj_size = h->next_free - h->object_base;
    char* object_base;

    /* Compute size for the new chunk.  We want enough for the existing
       object plus LENGTH more, aligned plus some extra. */
    {
        size_t sum1 = obj_size + length;
        size_t sum2 = sum1 + h->alignment_mask;
        size_t new_size = sum2 + (obj_size >> 3) + 100;
        if (new_size < sum2)
            new_size = sum2;
        if (new_size < h->chunk_size)
            new_size = h->chunk_size;

        if (obj_size <= sum1 && sum1 <= sum2)
            new_chunk = (struct _obstack_chunk*)call_chunkfun(h, new_size);
        if (!new_chunk)
            (*obstack_alloc_failed_handler)();
        new_chunk->prev = old_chunk;
        new_chunk->limit = (char*)new_chunk + new_size;
        h->chunk_limit = new_chunk->limit;
    }

    /* Align object base in the new chunk. */
    object_base = (char*)__PTR_ALIGN((char*)new_chunk, new_chunk->contents, h->alignment_mask);

    /* Copy existing object data to the new chunk. */
    memcpy(object_base, h->object_base, obj_size);

    /* Possibly free the old chunk if it contained only this one object. */
    if (!h->maybe_empty_object &&
        (h->object_base == (char*)__PTR_ALIGN((char*)old_chunk, old_chunk->contents, h->alignment_mask))) {
        new_chunk->prev = old_chunk->prev;
        call_freefun(h, old_chunk);
    }

    h->chunk = new_chunk;
    h->object_base = object_base;
    h->next_free = object_base + obj_size;
    h->maybe_empty_object = 0;
}

/*
 * _obstack_allocated_p -- debugging function; returns nonzero if OBJ
 * is within the chunks of obstack H.  Typically not used in production.
 */
int _obstack_allocated_p(struct obstack* h, void* obj) {
    struct _obstack_chunk* lp = h->chunk;

    while (lp != NULL && ((void*)lp >= obj || (void*)lp->limit < obj))
        lp = lp->prev;

    return lp != NULL;
}

/*
 * _obstack_free -- Free objects in obstack H, including OBJ and everything
 * allocated more recently. If OBJ == NULL, free everything.
 */
void _obstack_free(struct obstack* h, void* obj) {
    struct _obstack_chunk* lp = h->chunk;

    /* If obj is NULL, free everything. */
    if (obj == NULL) {
        /* Free all chunks. */
        while (lp) {
            struct _obstack_chunk* prev = lp->prev;
            call_freefun(h, lp);
            lp = prev;
        }
        /* Re-initialize obstack as empty. */
        h->chunk = NULL;
        return;
    }

    /* Otherwise free back to OBJ. */
    while (lp != NULL && ((void*)lp >= obj || (void*)lp->limit < obj)) {
        struct _obstack_chunk* prev = lp->prev;
        call_freefun(h, lp);
        lp = prev;
        h->maybe_empty_object = 1;
    }
    if (lp) {
        h->object_base = h->next_free = (char*)obj;
        h->chunk_limit = lp->limit;
        h->chunk = lp;
    }
    else {
        /* obj not found in any chunk => error. */
        abort();
    }
}

int obstack_printf(struct obstack* obstack, const char* __restrict fmt, ...) {
    char buf[1024];
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    obstack_grow(obstack, buf, len);
    va_end(ap);

    return len;
}

/*
 * _obstack_memory_used -- Return total memory (bytes) used by H.
 */
_OBSTACK_SIZE_T
_obstack_memory_used(struct obstack* h) {
    struct _obstack_chunk* lp;
    _OBSTACK_SIZE_T nbytes = 0;
    for (lp = h->chunk; lp; lp = lp->prev)
        nbytes += (lp->limit - (char*)lp);
    return nbytes;
}

#ifndef _OBSTACK_NO_ERROR_HANDLER
/*
 * Default error handler: print "memory exhausted" and exit nonzero.
 * If your environment wants a different behavior, you can override
 * obstack_alloc_failed_handler at runtime.
 */

static _Noreturn void print_and_abort(void) {
    fprintf(stderr, "memory exhausted\n");
    exit(EXIT_FAILURE);
}

/* The function pointer used by obstack to signal allocation failure.  */
void (*obstack_alloc_failed_handler)(void) = print_and_abort;

#endif /* !_OBSTACK_NO_ERROR_HANDLER */
