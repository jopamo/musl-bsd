#ifndef MY_OBSTACK_H
#define MY_OBSTACK_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _OBSTACK_INTERFACE_VERSION
#define _OBSTACK_INTERFACE_VERSION 2
#endif

#if !(defined _Noreturn || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112))
#if ((defined __GNUC__ && (__GNUC__ >= 3)) || (defined __SUNPRO_C && __SUNPRO_C >= 0x5110))
#define _Noreturn __attribute__((__noreturn__))
#elif defined _MSC_VER && _MSC_VER >= 1200
#define _Noreturn __declspec(noreturn)
#else
#define _Noreturn
#endif
#endif

#ifndef MY_OBSTACK_MAX
#define MY_OBSTACK_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT MY_OBSTACK_MAX(_Alignof(long double), MY_OBSTACK_MAX(_Alignof(uintmax_t), _Alignof(void*)))
#endif

#ifndef DEFAULT_ROUNDING
#define DEFAULT_ROUNDING MY_OBSTACK_MAX(sizeof(long double), MY_OBSTACK_MAX(sizeof(uintmax_t), sizeof(void*)))
#endif

#if _OBSTACK_INTERFACE_VERSION == 1
#define _OBSTACK_SIZE_T unsigned int
#define _CHUNK_SIZE_T unsigned long
#else
#define _OBSTACK_SIZE_T size_t
#define _CHUNK_SIZE_T size_t
#endif

struct _obstack_chunk {
    char* limit;
    struct _obstack_chunk* prev;
    char contents[4];
};

struct obstack {
    _CHUNK_SIZE_T chunk_size;
    struct _obstack_chunk* chunk;
    char* object_base;
    char* next_free;
    char* chunk_limit;
    union {
        _OBSTACK_SIZE_T i;
        void* p;
    } temp;
    _OBSTACK_SIZE_T alignment_mask;

    union {
        void* (*plain)(size_t);
        void* (*extra)(void*, size_t);
    } chunkfun;
    union {
        void (*plain)(void*);
        void (*extra)(void*, void*);
    } freefun;

    void* extra_arg;
    unsigned use_extra_arg : 1;
    unsigned maybe_empty_object : 1;
    unsigned alloc_failed : 1;
};

extern int _obstack_begin(struct obstack* h,
                          _OBSTACK_SIZE_T size,
                          _OBSTACK_SIZE_T alignment,
                          void* (*chunkfun)(size_t),
                          void (*freefun)(void*));

extern int _obstack_begin_1(struct obstack* h,
                            _OBSTACK_SIZE_T size,
                            _OBSTACK_SIZE_T alignment,
                            void* (*chunkfun)(void*, size_t),
                            void (*freefun)(void*, void*),
                            void* arg);

extern void _obstack_newchunk(struct obstack*, _OBSTACK_SIZE_T);

extern void _obstack_free(struct obstack*, void*);

extern _OBSTACK_SIZE_T _obstack_memory_used(struct obstack* h);

extern void (*obstack_alloc_failed_handler)(void);

extern int obstack_printf(struct obstack*, const char* __restrict, ...) __attribute__((format(printf, 2, 3)));

#ifndef obstack_chunk_alloc
#define obstack_chunk_alloc malloc
#endif
#ifndef obstack_chunk_free
#define obstack_chunk_free free
#endif

#define obstack_init(h) _obstack_begin((h), 0, 0, obstack_chunk_alloc, obstack_chunk_free)

#define obstack_begin(h, size) _obstack_begin((h), (size), 0, obstack_chunk_alloc, obstack_chunk_free)

#define obstack_specify_allocation(h, size, alignment, chunkfun, freefun) \
    _obstack_begin((h), (size), (alignment), (chunkfun), (freefun))

#define obstack_specify_allocation_with_arg(h, size, alignment, chunkfun, freefun, arg) \
    _obstack_begin_1((h), (size), (alignment), (chunkfun), (freefun), (arg))

#define obstack_base(h) ((void*)((h)->object_base))
#define obstack_next_free(h) ((void*)((h)->next_free))
#define obstack_chunk_size(h) ((h)->chunk_size)
#define obstack_alignment_mask(h) ((h)->alignment_mask)
#define obstack_memory_used(h) _obstack_memory_used(h)

#define obstack_object_size(h) ((_OBSTACK_SIZE_T)((h)->next_free - (h)->object_base))

#define obstack_room(h) ((_OBSTACK_SIZE_T)((h)->chunk_limit - (h)->next_free))

#define obstack_empty_p(h)    \
    ((h)->chunk->prev == 0 && \
     (h)->next_free == __PTR_ALIGN((char*)(h)->chunk, (h)->chunk->contents, (h)->alignment_mask))

#ifdef __GNUC__

#define obstack_make_room(h, length)       \
    __extension__({                        \
        struct obstack* __o = (h);         \
        _OBSTACK_SIZE_T __len = (length);  \
        if (obstack_room(__o) < __len)     \
            _obstack_newchunk(__o, __len); \
    })

#define obstack_grow(h, where, length)          \
    __extension__({                             \
        struct obstack* __o = (h);              \
        _OBSTACK_SIZE_T __len = (length);       \
        if (obstack_room(__o) < __len)          \
            _obstack_newchunk(__o, __len);      \
        memcpy(__o->next_free, (where), __len); \
        __o->next_free += __len;                \
    })

#define obstack_grow0(h, where, length)         \
    __extension__({                             \
        struct obstack* __o = (h);              \
        _OBSTACK_SIZE_T __len = (length);       \
        if (obstack_room(__o) < (__len + 1))    \
            _obstack_newchunk(__o, __len + 1);  \
        memcpy(__o->next_free, (where), __len); \
        __o->next_free += __len;                \
        *(__o->next_free)++ = 0;                \
    })

#define obstack_1grow(h, ch)           \
    __extension__({                    \
        struct obstack* __o = (h);     \
        if (obstack_room(__o) < 1)     \
            _obstack_newchunk(__o, 1); \
        *(__o->next_free)++ = (ch);    \
    })

#define obstack_blank(h, length)           \
    __extension__({                        \
        struct obstack* __o = (h);         \
        _OBSTACK_SIZE_T __len = (length);  \
        if (obstack_room(__o) < __len)     \
            _obstack_newchunk(__o, __len); \
        __o->next_free += __len;           \
    })

#define obstack_finish(h)                                                                   \
    __extension__({                                                                         \
        struct obstack* __o1 = (h);                                                         \
        void* __value = (void*)__o1->object_base;                                           \
        if (__o1->next_free == __value)                                                     \
            __o1->maybe_empty_object = 1;                                                   \
                                                                                            \
        {                                                                                   \
            size_t __aligned = (size_t)(__o1->alignment_mask);                              \
            char* __tmp = (char*)(((uintptr_t)(__o1->next_free) + __aligned) & ~__aligned); \
            if (__tmp > __o1->chunk_limit)                                                  \
                __tmp = __o1->chunk_limit;                                                  \
            __o1->next_free = __tmp;                                                        \
        }                                                                                   \
        __o1->object_base = __o1->next_free;                                                \
        __value;                                                                            \
    })

#define obstack_free(h, obj)                                                \
    __extension__({                                                         \
        struct obstack* __o = (h);                                          \
        void* __obj = (obj);                                                \
        if (__obj > (void*)__o->chunk && __obj < (void*)__o->chunk_limit) { \
            __o->next_free = __o->object_base = (char*)__obj;               \
        }                                                                   \
        else {                                                              \
            _obstack_free(__o, __obj);                                      \
        }                                                                   \
    })

#else

#define obstack_make_room(h, length)             \
    do {                                         \
        (h)->temp.i = (length);                  \
        if (obstack_room(h) < (h)->temp.i)       \
            _obstack_newchunk((h), (h)->temp.i); \
    } while (0)

#define obstack_grow(h, where, length)                \
    do {                                              \
        (h)->temp.i = (length);                       \
        if (obstack_room(h) < (h)->temp.i)            \
            _obstack_newchunk((h), (h)->temp.i);      \
        memcpy((h)->next_free, (where), (h)->temp.i); \
        (h)->next_free += (h)->temp.i;                \
    } while (0)

#define obstack_grow0(h, where, length)               \
    do {                                              \
        (h)->temp.i = (length);                       \
        if (obstack_room(h) < ((h)->temp.i + 1))      \
            _obstack_newchunk((h), (h)->temp.i + 1);  \
        memcpy((h)->next_free, (where), (h)->temp.i); \
        (h)->next_free += (h)->temp.i;                \
        *((h)->next_free)++ = 0;                      \
    } while (0)

#define obstack_1grow(h, ch)           \
    do {                               \
        if (obstack_room(h) < 1)       \
            _obstack_newchunk((h), 1); \
        *((h)->next_free)++ = (ch);    \
    } while (0)

#define obstack_blank(h, length)                 \
    do {                                         \
        (h)->temp.i = (length);                  \
        if (obstack_room(h) < (h)->temp.i)       \
            _obstack_newchunk((h), (h)->temp.i); \
        (h)->next_free += (h)->temp.i;           \
    } while (0)

#define obstack_finish(h)                                                                  \
    __extension__({                                                                        \
        void* __retval;                                                                    \
        if ((h)->next_free == (h)->object_base)                                            \
            (h)->maybe_empty_object = 1;                                                   \
                                                                                           \
        {                                                                                  \
            size_t __aligned = (size_t)((h)->alignment_mask);                              \
            char* __tmp = (char*)(((uintptr_t)((h)->next_free) + __aligned) & ~__aligned); \
            if (__tmp > (h)->chunk_limit)                                                  \
                __tmp = (h)->chunk_limit;                                                  \
            (h)->next_free = __tmp;                                                        \
        }                                                                                  \
        __retval = (h)->object_base;                                                       \
        (h)->object_base = (h)->next_free;                                                 \
        __retval;                                                                          \
    })

#define obstack_free(h, obj)                                                \
    do {                                                                    \
        void* __obj = (obj);                                                \
        if (__obj > (void*)(h)->chunk && __obj < (void*)(h)->chunk_limit) { \
            (h)->next_free = (h)->object_base = (char*)__obj;               \
        }                                                                   \
        else {                                                              \
            _obstack_free((h), __obj);                                      \
        }                                                                   \
    } while (0)

#endif

#ifdef __cplusplus
}
#endif

#endif
