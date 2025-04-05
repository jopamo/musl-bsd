/* obstack.h
 */

#ifndef _OBSTACK_H
#define _OBSTACK_H 1

#ifndef _OBSTACK_INTERFACE_VERSION
#define _OBSTACK_INTERFACE_VERSION 2
#endif

#include <stddef.h> /* For size_t, ptrdiff_t */
#include <string.h> /* For memcpy */
#include <stdlib.h> /* For EXIT_FAILURE, if needed. */

/* For backward compatibility, we define certain types differently
   depending on the interface version.  In general, version 2 uses
   size_t for both chunk size and object size. */
#if _OBSTACK_INTERFACE_VERSION == 1
/* Old style uses "int" and "long". */
#define _OBSTACK_SIZE_T unsigned int
#define _CHUNK_SIZE_T unsigned long
#define _OBSTACK_CAST(type, expr) ((type)(expr))
#else
/* New style uses size_t. */
#define _OBSTACK_SIZE_T size_t
#define _CHUNK_SIZE_T size_t
#define _OBSTACK_CAST(type, expr) (expr)
#endif

/*
   Macro for pointer alignment:
   Align pointer P up to the next boundary given by A + 1,
   assuming A+1 is a power of two. B is the chunk base, needed in
   some fallback logic. We define a faster version that relies on
   casting P to ptrdiff_t if that is as wide as a pointer.
*/
#define __BPTR_ALIGN(B, P, A) ((B) + ((((P) - (B)) + (A)) & ~(A)))

#define __PTR_ALIGN(B, P, A) \
    (sizeof(ptrdiff_t) < sizeof(void*) ? __BPTR_ALIGN(B, P, A) : (char*)(((ptrdiff_t)(P) + (A)) & ~(A)))

/* Some compilers define __attribute__pure__ differently. Fallback: none. */
#ifndef __attribute_pure__
#if defined __GNUC__ && (__GNUC__ >= 2)
#define __attribute_pure__ __attribute__((__pure__))
#else
#define __attribute_pure__
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
   Each chunk starts with this header.
   'contents' is the base address where objects can actually be placed.
*/
struct _obstack_chunk {
    char* limit;                 /* 1 past end of this chunk */
    struct _obstack_chunk* prev; /* Pointer to previous chunk */
    char contents[4];            /* Beginning of data space for objects */
};

/*
   The obstack control structure keeps track of the current chunk,
   current object, alignment, etc. Each object is grown in place until
   it is finalized. Many macros operate on 'struct obstack *'.
*/
struct obstack {
    _CHUNK_SIZE_T chunk_size;     /* preferred size for new chunks */
    struct _obstack_chunk* chunk; /* current chunk pointer */
    char* object_base;            /* address of object we are building */
    char* next_free;              /* where to add next byte to current object */
    char* chunk_limit;            /* address of char after current chunk */
    union {
        _OBSTACK_SIZE_T i;
        void* p;
    } temp;                         /* Temporary space for macros. */
    _OBSTACK_SIZE_T alignment_mask; /* low bits that must be clear in ptrs */

    /* We can store either the normal or "extra arg" versions of the allocation
       functions. The field 'use_extra_arg' says which is in use. */
    union {
        void* (*plain)(size_t);
        void* (*extra)(void*, size_t);
    } chunkfun;
    union {
        void (*plain)(void*);
        void (*extra)(void*, void*);
    } freefun;

    void* extra_arg;                 /* optional argument for chunkfun/freefun */
    unsigned use_extra_arg : 1;      /* if true, use the 'extra' version of alloc/free */
    unsigned maybe_empty_object : 1; /* if true, chunk might contain a zero-length object */
    unsigned alloc_failed : 1;       /* not really used now, but kept for binary compat */
};

/*------------------- Function Prototypes -------------------*/

/* Obtain a new chunk if we run out of room.  */
extern void _obstack_newchunk(struct obstack*, _OBSTACK_SIZE_T);

/* Free objects in H including OBJ and any allocated after OBJ.
   If OBJ is NULL, free everything in H. */
extern void _obstack_free(struct obstack*, void*);

/* Start an obstack using chunkfun/freefun that take no extra arg. */
extern int _obstack_begin(struct obstack*, _OBSTACK_SIZE_T, _OBSTACK_SIZE_T, void* (*)(size_t), void (*)(void*));

/* Start an obstack using chunkfun/freefun that do take an extra arg. */
extern int _obstack_begin_1(struct obstack*,
                            _OBSTACK_SIZE_T,
                            _OBSTACK_SIZE_T,
                            void* (*)(void*, size_t),
                            void (*)(void*, void*),
                            void* /* arg */);

/* Return total bytes used by the obstack (for diagnostic/debug). */
extern _OBSTACK_SIZE_T _obstack_memory_used(struct obstack*) __attribute_pure__;

/* Handler called if allocation (via chunkfun) fails. It must not return. */
extern void (*obstack_alloc_failed_handler)(void);

/* Exit value used by the default handler. */
extern int obstack_exit_failure;

/*------------------- Basic Helper Macros -------------------*/

/* Return pointer to beginning of the currently built object.  */
#define obstack_base(h) ((void*)((h)->object_base))

/* Return pointer to next unoccupied byte in the current chunk.  */
#define obstack_next_free(h) ((void*)((h)->next_free))

/* Return the current chunk size (the "preferred" chunk size). */
#define obstack_chunk_size(h) ((h)->chunk_size)

/* Return the alignment mask for this obstack. */
#define obstack_alignment_mask(h) ((h)->alignment_mask)

/*
   Traditional approach to quickly initialize an obstack with
   some standard allocation/free routines (like `malloc`/`free`).
   By default, `obstack_chunk_alloc` and `obstack_chunk_free` should
   be macros or function pointers that the user defines or references.

   Example (if you want standard malloc):
       #define obstack_chunk_alloc  malloc
       #define obstack_chunk_free   free
   Then:
       obstack_init(my_obstack_pointer);
*/
#define obstack_init(h)                                                              \
    _obstack_begin((h), 0, 0, _OBSTACK_CAST(void* (*)(size_t), obstack_chunk_alloc), \
                   _OBSTACK_CAST(void (*)(void*), obstack_chunk_free))

/* Like obstack_init but explicitly specify the initial chunk size. */
#define obstack_begin(h, size)                                                            \
    _obstack_begin((h), (size), 0, _OBSTACK_CAST(void* (*)(size_t), obstack_chunk_alloc), \
                   _OBSTACK_CAST(void (*)(void*), obstack_chunk_free))

/* More advanced initialization macros that let you override alignment
   or supply custom chunk/free functions with or without an extra argument. */
#define obstack_specify_allocation(h, size, alignment, chunkfun, freefun)                  \
    _obstack_begin((h), (size), (alignment), _OBSTACK_CAST(void* (*)(size_t), (chunkfun)), \
                   _OBSTACK_CAST(void (*)(void*), (freefun)))

#define obstack_specify_allocation_with_arg(h, size, alignment, chunkfun, freefun, arg)             \
    _obstack_begin_1((h), (size), (alignment), _OBSTACK_CAST(void* (*)(void*, size_t), (chunkfun)), \
                     _OBSTACK_CAST(void (*)(void*, void*), (freefun)), (arg))

/* Overwrite the chunkfun/freefun in an existing obstack. */
#define obstack_chunkfun(h, newchunkfun) ((void)((h)->chunkfun.extra = (void* (*)(void*, size_t))(newchunkfun)))

#define obstack_freefun(h, newfreefun) ((void)((h)->freefun.extra = (void (*)(void*, void*))(newfreefun)))

/* Grow the current object by 1 char quickly (assuming enough space). */
#define obstack_1grow_fast(h, achar) ((void)(*((h)->next_free)++ = (achar)))

/* Advance the "next_free" pointer by n bytes quickly. */
#define obstack_blank_fast(h, n) ((void)((h)->next_free += (n)))

/* Return how many bytes are currently used in the object being built. */
#define obstack_object_size(h) ((_OBSTACK_SIZE_T)((h)->next_free - (h)->object_base))

/* Return how many bytes remain available in the current chunk. */
#define obstack_room(h) ((_OBSTACK_SIZE_T)((h)->chunk_limit - (h)->next_free))

/* Return non-zero if obstack is empty. */
#define obstack_empty_p(h)    \
    ((h)->chunk->prev == 0 && \
     (h)->next_free == __PTR_ALIGN((char*)(h)->chunk, (h)->chunk->contents, (h)->alignment_mask))

/*---------------------------------------------
  Additional macros (grow/grow0/etc.) differ
  slightly depending on whether we have GCC
  extensions or not. We keep them here for clarity.
---------------------------------------------*/

#if defined __GNUC__
/* If using GCC, we can do some expressions with blocks. */

/* Make room for LENGTH bytes, possibly allocating a new chunk. */
#define obstack_make_room(h, length)       \
    __extension__({                        \
        struct obstack* __o = (h);         \
        _OBSTACK_SIZE_T __len = (length);  \
        if (obstack_room(__o) < __len)     \
            _obstack_newchunk(__o, __len); \
        (void)0;                           \
    })

#define obstack_grow(h, where, length)          \
    __extension__({                             \
        struct obstack* __o = (h);              \
        _OBSTACK_SIZE_T __len = (length);       \
        if (obstack_room(__o) < __len)          \
            _obstack_newchunk(__o, __len);      \
        memcpy(__o->next_free, (where), __len); \
        __o->next_free += __len;                \
        (void)0;                                \
    })

#define obstack_grow0(h, where, length)         \
    __extension__({                             \
        struct obstack* __o = (h);              \
        _OBSTACK_SIZE_T __len = (length);       \
        if (obstack_room(__o) < __len + 1)      \
            _obstack_newchunk(__o, __len + 1);  \
        memcpy(__o->next_free, (where), __len); \
        __o->next_free += __len;                \
        *(__o->next_free)++ = 0;                \
        (void)0;                                \
    })

#define obstack_1grow(h, datum)         \
    __extension__({                     \
        struct obstack* __o = (h);      \
        if (obstack_room(__o) < 1)      \
            _obstack_newchunk(__o, 1);  \
        obstack_1grow_fast(__o, datum); \
    })

/* Pointers or int growth, relying on alignment. */
#define obstack_ptr_grow(h, datum)                 \
    __extension__({                                \
        struct obstack* __o = (h);                 \
        if (obstack_room(__o) < sizeof(void*))     \
            _obstack_newchunk(__o, sizeof(void*)); \
        obstack_ptr_grow_fast(__o, datum);         \
    })

#define obstack_int_grow(h, datum)               \
    __extension__({                              \
        struct obstack* __o = (h);               \
        if (obstack_room(__o) < sizeof(int))     \
            _obstack_newchunk(__o, sizeof(int)); \
        obstack_int_grow_fast(__o, datum);       \
    })

/* "Fast" versions assume enough room. */
#define obstack_ptr_grow_fast(h, aptr)          \
    __extension__({                             \
        struct obstack* __o1 = (h);             \
        void* __p1 = __o1->next_free;           \
        *(const void**)__p1 = (aptr);           \
        __o1->next_free += sizeof(const void*); \
        (void)0;                                \
    })

#define obstack_int_grow_fast(h, aint)  \
    __extension__({                     \
        struct obstack* __o1 = (h);     \
        void* __p1 = __o1->next_free;   \
        *(int*)__p1 = (aint);           \
        __o1->next_free += sizeof(int); \
        (void)0;                        \
    })

#define obstack_blank(h, length)           \
    __extension__({                        \
        struct obstack* __o = (h);         \
        _OBSTACK_SIZE_T __len = (length);  \
        if (obstack_room(__o) < __len)     \
            _obstack_newchunk(__o, __len); \
        obstack_blank_fast(__o, __len);    \
    })

#define obstack_alloc(h, length)      \
    __extension__({                   \
        struct obstack* __o = (h);    \
        obstack_blank(__o, (length)); \
        obstack_finish(__o);          \
    })

#define obstack_copy(h, where, length)        \
    __extension__({                           \
        struct obstack* __o = (h);            \
        obstack_grow(__o, (where), (length)); \
        obstack_finish(__o);                  \
    })

#define obstack_copy0(h, where, length)        \
    __extension__({                            \
        struct obstack* __o = (h);             \
        obstack_grow0(__o, (where), (length)); \
        obstack_finish(__o);                   \
    })

/* Finalize the object: align next_free and set object_base to next_free. */
#define obstack_finish(h)                                                                                      \
    __extension__({                                                                                            \
        struct obstack* __o1 = (h);                                                                            \
        void* __value = (void*)__o1->object_base;                                                              \
        if (__o1->next_free == __value)                                                                        \
            __o1->maybe_empty_object = 1;                                                                      \
        __o1->next_free = __PTR_ALIGN(__o1->object_base, __o1->next_free, __o1->alignment_mask);               \
        if ((size_t)(__o1->next_free - (char*)__o1->chunk) > (size_t)(__o1->chunk_limit - (char*)__o1->chunk)) \
            __o1->next_free = __o1->chunk_limit;                                                               \
        __o1->object_base = __o1->next_free;                                                                   \
        __value;                                                                                               \
    })

#define obstack_free(h, obj)                                              \
    __extension__({                                                       \
        struct obstack* __o = (h);                                        \
        void* __obj = (void*)(obj);                                       \
        if (__obj > (void*)__o->chunk && __obj < (void*)__o->chunk_limit) \
            __o->next_free = __o->object_base = (char*)__obj;             \
        else                                                              \
            _obstack_free(__o, __obj);                                    \
    })

#else /* not __GNUC__ */

/* Fallback definitions for compilers without GCC extensions. */

#define obstack_make_room(h, length) \
    ((h)->temp.i = (length), ((obstack_room(h) < (h)->temp.i) ? (_obstack_newchunk((h), (h)->temp.i), 0) : 0), (void)0)

#define obstack_grow(h, where, length)                                                                         \
    ((h)->temp.i = (length), ((obstack_room(h) < (h)->temp.i) ? (_obstack_newchunk((h), (h)->temp.i), 0) : 0), \
     memcpy((h)->next_free, (where), (h)->temp.i), (h)->next_free += (h)->temp.i, (void)0)

#define obstack_grow0(h, where, length)                                                                                \
    ((h)->temp.i = (length), ((obstack_room(h) < (h)->temp.i + 1) ? (_obstack_newchunk((h), (h)->temp.i + 1), 0) : 0), \
     memcpy((h)->next_free, (where), (h)->temp.i), (h)->next_free += (h)->temp.i, *((h)->next_free)++ = 0, (void)0)

#define obstack_1grow(h, datum) \
    (((obstack_room(h) < 1) ? (_obstack_newchunk((h), 1), 0) : 0), obstack_1grow_fast((h), (datum)))

#define obstack_ptr_grow(h, datum)                                                         \
    (((obstack_room(h) < sizeof(void*)) ? (_obstack_newchunk((h), sizeof(void*)), 0) : 0), \
     obstack_ptr_grow_fast((h), (datum)))

#define obstack_int_grow(h, datum)                                                     \
    (((obstack_room(h) < sizeof(int)) ? (_obstack_newchunk((h), sizeof(int)), 0) : 0), \
     obstack_int_grow_fast((h), (datum)))

#define obstack_ptr_grow_fast(h, aptr) (((const void**)((h)->next_free += sizeof(void*)))[-1] = (aptr), (void)0)

#define obstack_int_grow_fast(h, aint) (((int*)((h)->next_free += sizeof(int)))[-1] = (aint), (void)0)

#define obstack_blank(h, length)                                                                               \
    ((h)->temp.i = (length), ((obstack_room(h) < (h)->temp.i) ? (_obstack_newchunk((h), (h)->temp.i), 0) : 0), \
     obstack_blank_fast((h), (h)->temp.i))

#define obstack_alloc(h, length) (obstack_blank((h), (length)), obstack_finish((h)))

#define obstack_copy(h, where, length) (obstack_grow((h), (where), (length)), obstack_finish((h)))

#define obstack_copy0(h, where, length) (obstack_grow0((h), (where), (length)), obstack_finish((h)))

#define obstack_finish(h)                                                                                         \
    (((h)->next_free == (h)->object_base ? ((h)->maybe_empty_object = 1, 0) : 0), (h)->temp.p = (h)->object_base, \
     (h)->next_free = __PTR_ALIGN((h)->object_base, (h)->next_free, (h)->alignment_mask),                         \
     (((size_t)((h)->next_free - (char*)(h)->chunk) > (size_t)((h)->chunk_limit - (char*)(h)->chunk))             \
          ? ((h)->next_free = (h)->chunk_limit)                                                                   \
          : 0),                                                                                                   \
     (h)->object_base = (h)->next_free, (h)->temp.p)

#define obstack_free(h, obj)                                                                                 \
    ((h)->temp.p = (void*)(obj), (((h)->temp.p > (void*)(h)->chunk && (h)->temp.p < (void*)(h)->chunk_limit) \
                                      ? (void)((h)->next_free = (h)->object_base = (char*)(h)->temp.p)       \
                                      : _obstack_free((h), (h)->temp.p)))

#endif /* __GNUC__ */

/* Provide a macro to query how many bytes are used by the entire obstack. */
#define obstack_memory_used(h) _obstack_memory_used(h)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _OBSTACK_H */
