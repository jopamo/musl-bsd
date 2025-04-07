#ifndef _OBSTACK_H
#define _OBSTACK_H 1

#ifndef _OBSTACK_INTERFACE_VERSION
#define _OBSTACK_INTERFACE_VERSION 2
#endif

#include <stddef.h>
#include <string.h>

#if _OBSTACK_INTERFACE_VERSION == 1
#define _OBSTACK_SIZE_T unsigned int
#define _CHUNK_SIZE_T unsigned long
#define _OBSTACK_CAST(type, expr) ((type)(expr))
#else
#define _OBSTACK_SIZE_T size_t
#define _CHUNK_SIZE_T size_t
#define _OBSTACK_CAST(type, expr) (expr)
#endif

#ifndef __BPTR_ALIGN
#define __BPTR_ALIGN(B, P, A) ((B) + ((((P) - (B)) + (A)) & ~((A))))
#endif

#ifndef __PTR_ALIGN
#define __PTR_ALIGN(B, P, A) \
    (sizeof(ptrdiff_t) < sizeof(void*) ? __BPTR_ALIGN(B, P, A) : (char*)(((ptrdiff_t)(P) + (A)) & ~(A)))
#endif

#ifndef __attribute_pure__
#if defined __GNUC_MINOR__ && __GNUC__ * 1000 + __GNUC_MINOR__ >= 2096
#define __attribute_pure__ __attribute__((__pure__))
#else
#define __attribute_pure__
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT 16
#endif

#ifndef DEFAULT_ROUNDING
#define DEFAULT_ROUNDING (DEFAULT_ALIGNMENT - 1)
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

extern int _obstack_begin(struct obstack*, _OBSTACK_SIZE_T, _OBSTACK_SIZE_T, void* (*)(size_t), void (*)(void*));
extern int _obstack_begin_1(struct obstack*,
                            _OBSTACK_SIZE_T,
                            _OBSTACK_SIZE_T,
                            void* (*)(void*, size_t),
                            void (*)(void*, void*),
                            void*);
extern void _obstack_newchunk(struct obstack*, _OBSTACK_SIZE_T);
extern void _obstack_free(struct obstack*, void*);
extern _OBSTACK_SIZE_T _obstack_memory_used(struct obstack*) __attribute_pure__;

extern void (*obstack_alloc_failed_handler)(void);

extern int obstack_exit_failure;

extern int obstack_printf(struct obstack*, const char* __restrict, ...) __attribute__((format(printf, 2, 3)));

#ifndef obstack_chunk_alloc
#define obstack_chunk_alloc malloc
#endif
#ifndef obstack_chunk_free
#define obstack_chunk_free free
#endif

#define obstack_init(h)                                                              \
    _obstack_begin((h), 0, 0, _OBSTACK_CAST(void* (*)(size_t), obstack_chunk_alloc), \
                   _OBSTACK_CAST(void (*)(void*), obstack_chunk_free))

#define obstack_begin(h, size)                                                            \
    _obstack_begin((h), (size), 0, _OBSTACK_CAST(void* (*)(size_t), obstack_chunk_alloc), \
                   _OBSTACK_CAST(void (*)(void*), obstack_chunk_free))

#define obstack_specify_allocation(h, size, alignment, chunkfun, freefun)                  \
    _obstack_begin((h), (size), (alignment), _OBSTACK_CAST(void* (*)(size_t), (chunkfun)), \
                   _OBSTACK_CAST(void (*)(void*), (freefun)))

#define obstack_specify_allocation_with_arg(h, size, alignment, chunkfun, freefun, arg)             \
    _obstack_begin_1((h), (size), (alignment), _OBSTACK_CAST(void* (*)(void*, size_t), (chunkfun)), \
                     _OBSTACK_CAST(void (*)(void*, void*), (freefun)), (arg))

#define obstack_chunkfun(h, newfun) ((void)((h)->chunkfun.extra = (void* (*)(void*, size_t))(newfun)))

#define obstack_freefun(h, newfun) ((void)((h)->freefun.extra = (void (*)(void*, void*))(newfun)))

#define obstack_base(h) ((void*)(h)->object_base)
#define obstack_next_free(h) ((void*)(h)->next_free)
#define obstack_chunk_size(h) ((h)->chunk_size)
#define obstack_alignment_mask(h) ((h)->alignment_mask)
#define obstack_memory_used(h) _obstack_memory_used(h)

#define obstack_int_grow(H, aint)                                                      \
    (((obstack_room(H) < sizeof(int)) ? (_obstack_newchunk((H), sizeof(int)), 0) : 0), \
     ((*((int*)((H)->next_free += sizeof(int))) = (aint)), 0))

#if defined __GNUC__
#if !defined __GNUC_MINOR__ || __GNUC__ * 1000 + __GNUC_MINOR__ < 2008
#define __extension__
#endif

#define obstack_object_size(H)                                \
    __extension__({                                           \
        struct obstack const* __o = (H);                      \
        (_OBSTACK_SIZE_T)(__o->next_free - __o->object_base); \
    })

#define obstack_room(H)                                         \
    __extension__({                                             \
        struct obstack const* __o1 = (H);                       \
        (_OBSTACK_SIZE_T)(__o1->chunk_limit - __o1->next_free); \
    })

#define obstack_make_room(H, length)       \
    __extension__({                        \
        struct obstack* __o = (H);         \
        _OBSTACK_SIZE_T __len = (length);  \
        if (obstack_room(__o) < __len)     \
            _obstack_newchunk(__o, __len); \
    })

#define obstack_empty_p(H)                                                                             \
    __extension__({                                                                                    \
        struct obstack const* __o = (H);                                                               \
        (__o->chunk->prev == 0 &&                                                                      \
         __o->next_free == __PTR_ALIGN((char*)__o->chunk, __o->chunk->contents, __o->alignment_mask)); \
    })

#define obstack_grow(H, where, length)          \
    __extension__({                             \
        struct obstack* __o = (H);              \
        _OBSTACK_SIZE_T __len = (length);       \
        if (obstack_room(__o) < __len)          \
            _obstack_newchunk(__o, __len);      \
        memcpy(__o->next_free, (where), __len); \
        __o->next_free += __len;                \
    })

#define obstack_grow0(H, where, length)         \
    __extension__({                             \
        struct obstack* __o = (H);              \
        _OBSTACK_SIZE_T __len = (length);       \
        if (obstack_room(__o) < __len + 1)      \
            _obstack_newchunk(__o, __len + 1);  \
        memcpy(__o->next_free, (where), __len); \
        __o->next_free += __len;                \
        *(__o->next_free)++ = 0;                \
    })

#define obstack_1grow(H, ch)           \
    __extension__({                    \
        struct obstack* __o = (H);     \
        if (obstack_room(__o) < 1)     \
            _obstack_newchunk(__o, 1); \
        *(__o->next_free)++ = (ch);    \
    })

#define obstack_blank(H, length)           \
    __extension__({                        \
        struct obstack* __o = (H);         \
        _OBSTACK_SIZE_T __len = (length);  \
        if (obstack_room(__o) < __len)     \
            _obstack_newchunk(__o, __len); \
        __o->next_free += __len;           \
    })

#define obstack_finish(H)                                                       \
    __extension__({                                                             \
        struct obstack* __o1 = (H);                                             \
        void* __value = (void*)__o1->object_base;                               \
        if (__o1->next_free == __o1->object_base)                               \
            __o1->maybe_empty_object = 1;                                       \
        {                                                                       \
            size_t __am = (size_t)__o1->alignment_mask;                         \
            char* __tmp = (char*)(((uintptr_t)__o1->next_free + __am) & ~__am); \
            if (__tmp > __o1->chunk_limit)                                      \
                __tmp = __o1->chunk_limit;                                      \
            __o1->next_free = __tmp;                                            \
        }                                                                       \
        __o1->object_base = __o1->next_free;                                    \
        __value;                                                                \
    })

#define obstack_free(H, OBJ)                                                     \
    __extension__({                                                              \
        struct obstack* __o = (H);                                               \
        void* __obj = (void*)(OBJ);                                              \
        if ((char*)__obj > (char*)__o->chunk && (char*)__obj < __o->chunk_limit) \
            __o->next_free = __o->object_base = (char*)__obj;                    \
        else                                                                     \
            _obstack_free(__o, __obj);                                           \
    })

#else

#define obstack_object_size(H) ((_OBSTACK_SIZE_T)((H)->next_free - (H)->object_base))

#define obstack_room(H) ((_OBSTACK_SIZE_T)((H)->chunk_limit - (H)->next_free))

#define obstack_empty_p(H)    \
    ((H)->chunk->prev == 0 && \
     ((H)->next_free == __PTR_ALIGN((char*)(H)->chunk, (H)->chunk->contents, (H)->alignment_mask)))

#define obstack_make_room(H, length) \
    ((H)->temp.i = (length), (obstack_room(H) < (H)->temp.i ? (_obstack_newchunk((H), (H)->temp.i), 0) : 0), (void)0)

#define obstack_grow(H, where, length)                                                                       \
    ((H)->temp.i = (length), (obstack_room(H) < (H)->temp.i ? (_obstack_newchunk((H), (H)->temp.i), 0) : 0), \
     memcpy((H)->next_free, (where), (H)->temp.i), ((H)->next_free += (H)->temp.i))

#define obstack_grow0(H, where, length)                                                                              \
    ((H)->temp.i = (length), (obstack_room(H) < (H)->temp.i + 1 ? (_obstack_newchunk((H), (H)->temp.i + 1), 0) : 0), \
     memcpy((H)->next_free, (where), (H)->temp.i), ((H)->next_free += (H)->temp.i), *((H)->next_free)++ = 0)

#define obstack_1grow(H, ch) \
    (((obstack_room(H) < 1) ? (_obstack_newchunk((H), 1), 0) : 0), (*((H)->next_free)++ = (ch)))

#define obstack_ptr_grow(H, aptr)                                                          \
    (((obstack_room(H) < sizeof(void*)) ? (_obstack_newchunk((H), sizeof(void*)), 0) : 0), \
     ((*((void**)((H)->next_free += sizeof(void*))) = (aptr)), 0))

#define obstack_blank(H, length)                                                                             \
    ((H)->temp.i = (length), (obstack_room(H) < (H)->temp.i ? (_obstack_newchunk((H), (H)->temp.i), 0) : 0), \
     ((H)->next_free += (H)->temp.i))

#define obstack_alloc(H, length) (obstack_blank((H), (length)), obstack_finish(H))

#define obstack_copy(H, where, length) (obstack_grow((H), (where), (length)), obstack_finish(H))

#define obstack_copy0(H, where, length) (obstack_grow0((H), (where), (length)), obstack_finish(H))

#define obstack_finish(H)                                                                                      \
    (((H)->next_free == (H)->object_base ? ((H)->maybe_empty_object = 1) : 0), (H)->temp.p = (H)->object_base, \
     (H)->next_free = __PTR_ALIGN((H)->object_base, (H)->next_free, (H)->alignment_mask),                      \
     (((size_t)((H)->next_free - (char*)(H)->chunk) > (size_t)((H)->chunk_limit - (char*)(H)->chunk))          \
          ? ((H)->next_free = (H)->chunk_limit)                                                                \
          : 0),                                                                                                \
     (H)->object_base = (H)->next_free, (H)->temp.p)

#define obstack_free(H, OBJ)                                                                                 \
    ((H)->temp.p = (void*)(OBJ), (((H)->temp.p > (void*)(H)->chunk && (H)->temp.p < (void*)(H)->chunk_limit) \
                                      ? (void)((H)->next_free = (H)->object_base = (char*)(H)->temp.p)       \
                                      : _obstack_free((H), (H)->temp.p)))
#endif

#ifdef __cplusplus
}
#endif

#endif
