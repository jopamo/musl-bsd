/*
 * queue.h
 *
 * Provides:
 *   - SLIST   (singly-linked list)
 *   - LIST    (doubly-linked list)
 *   - SIMPLEQ (simple FIFO queue with single links)
 *   - STAILQ  (singly-linked tail queue)
 *   - TAILQ   (tail queue with full double links)
 *   - XSIMPLEQ (XOR simple queue; requires entropy from getrandom())
 *
 */

#ifndef _SYS_QUEUE_H_
#define _SYS_QUEUE_H_

#include <stddef.h>     /* for NULL, offsetof */
#include <sys/random.h> /* for getrandom() */

/*
 * If you want to debug use-after-free errors, redefine this before
 * including queue.h. For example:
 *
 *   #define _Q_INVALIDATE(a)  ((a) = (void *)-1)
 */
#ifndef _Q_INVALIDATE
#define _Q_INVALIDATE(a) /* no-op by default */
#endif

/******************************************************************************
 *                          Helper: getrandom() wrapper
 ******************************************************************************/

static inline void queue_getrandom(void* buf, size_t len) {
    /*
     * We use a simple loop to ensure we read 'len' bytes total, as getrandom()
     * may return partial data. In production, you might also handle EINTR,
     * EAGAIN, etc., but typically blocking until enough entropy is available
     * is correct for cryptographic usage.
     */
    unsigned char* p = buf;
    size_t bytes_read = 0;
    while (bytes_read < len) {
        ssize_t ret = getrandom(p + bytes_read, len - bytes_read, 0);
        if (ret < 0) {
            /*
             * Production code might handle errors carefully. For simplicity,
             * we just set all bytes to zero and return. You can also abort()
             * or fallback to /dev/urandom, etc.
             */
            while (bytes_read < len) {
                p[bytes_read++] = 0;
            }
            return;
        }
        bytes_read += (size_t)ret;
    }
}

/******************************************************************************
 *                            Singly-linked List (SLIST)
 ******************************************************************************/
#define SLIST_HEAD(name, type)                      \
    struct name {                                   \
        struct type* slh_first; /* first element */ \
    }

#define SLIST_HEAD_INITIALIZER(head) {NULL}

#define SLIST_ENTRY(type)                         \
    struct {                                      \
        struct type* sle_next; /* next element */ \
    }

/* Access methods */
#define SLIST_FIRST(head) ((head)->slh_first)
#define SLIST_END(head) NULL
#define SLIST_EMPTY(head) (SLIST_FIRST((head)) == SLIST_END(head))
#define SLIST_NEXT(elm, field) ((elm)->field.sle_next)

/* Traversal */
#define SLIST_FOREACH(var, head, field) \
    for ((var) = SLIST_FIRST((head)); (var) != SLIST_END(head); (var) = SLIST_NEXT((var), field))

#define SLIST_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = SLIST_FIRST((head)); (var) && ((tvar) = SLIST_NEXT((var), field), 1); (var) = (tvar))

/* Functions */
#define SLIST_INIT(head)                       \
    do {                                       \
        SLIST_FIRST((head)) = SLIST_END(head); \
    } while (0)

#define SLIST_INSERT_AFTER(slistelm, elm, field)            \
    do {                                                    \
        (elm)->field.sle_next = (slistelm)->field.sle_next; \
        (slistelm)->field.sle_next = (elm);                 \
    } while (0)

#define SLIST_INSERT_HEAD(head, elm, field)        \
    do {                                           \
        (elm)->field.sle_next = (head)->slh_first; \
        (head)->slh_first = (elm);                 \
    } while (0)

#define SLIST_REMOVE_AFTER(elm, field)                                 \
    do {                                                               \
        (elm)->field.sle_next = (elm)->field.sle_next->field.sle_next; \
    } while (0)

#define SLIST_REMOVE_HEAD(head, field)                         \
    do {                                                       \
        (head)->slh_first = (head)->slh_first->field.sle_next; \
    } while (0)

#define SLIST_REMOVE(head, elm, type, field)                                 \
    do {                                                                     \
        if ((head)->slh_first == (elm)) {                                    \
            SLIST_REMOVE_HEAD((head), field);                                \
        }                                                                    \
        else {                                                               \
            struct type* curelm = (head)->slh_first;                         \
            while (curelm->field.sle_next != (elm))                          \
                curelm = curelm->field.sle_next;                             \
            curelm->field.sle_next = curelm->field.sle_next->field.sle_next; \
        }                                                                    \
        _Q_INVALIDATE((elm)->field.sle_next);                                \
    } while (0)

/******************************************************************************
 *                            Doubly-linked List (LIST)
 ******************************************************************************/
#define LIST_HEAD(name, type)                      \
    struct name {                                  \
        struct type* lh_first; /* first element */ \
    }

#define LIST_HEAD_INITIALIZER(head) {NULL}

#define LIST_ENTRY(type)                                  \
    struct {                                              \
        struct type* le_next;  /* next element */         \
        struct type** le_prev; /* ptr to previous link */ \
    }

/* Access methods */
#define LIST_FIRST(head) ((head)->lh_first)
#define LIST_END(head) NULL
#define LIST_EMPTY(head) (LIST_FIRST((head)) == LIST_END(head))
#define LIST_NEXT(elm, field) ((elm)->field.le_next)

/* Traversal */
#define LIST_FOREACH(var, head, field) \
    for ((var) = LIST_FIRST((head)); (var) != LIST_END(head); (var) = LIST_NEXT((var), field))

#define LIST_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = LIST_FIRST((head)); (var) && ((tvar) = LIST_NEXT((var), field), 1); (var) = (tvar))

/* Functions */
#define LIST_INIT(head)                      \
    do {                                     \
        LIST_FIRST((head)) = LIST_END(head); \
    } while (0)

#define LIST_INSERT_AFTER(listelm, elm, field)                               \
    do {                                                                     \
        if (((elm)->field.le_next = (listelm)->field.le_next) != NULL)       \
            (listelm)->field.le_next->field.le_prev = &(elm)->field.le_next; \
        (listelm)->field.le_next = (elm);                                    \
        (elm)->field.le_prev = &(listelm)->field.le_next;                    \
    } while (0)

#define LIST_INSERT_BEFORE(listelm, elm, field)           \
    do {                                                  \
        (elm)->field.le_prev = (listelm)->field.le_prev;  \
        (elm)->field.le_next = (listelm);                 \
        *(listelm)->field.le_prev = (elm);                \
        (listelm)->field.le_prev = &(elm)->field.le_next; \
    } while (0)

#define LIST_INSERT_HEAD(head, elm, field)                           \
    do {                                                             \
        if (((elm)->field.le_next = (head)->lh_first) != NULL)       \
            (head)->lh_first->field.le_prev = &(elm)->field.le_next; \
        (head)->lh_first = (elm);                                    \
        (elm)->field.le_prev = &(head)->lh_first;                    \
    } while (0)

#define LIST_REMOVE(elm, field)                                         \
    do {                                                                \
        if ((elm)->field.le_next != NULL)                               \
            (elm)->field.le_next->field.le_prev = (elm)->field.le_prev; \
        *(elm)->field.le_prev = (elm)->field.le_next;                   \
        _Q_INVALIDATE((elm)->field.le_prev);                            \
        _Q_INVALIDATE((elm)->field.le_next);                            \
    } while (0)

#define LIST_REPLACE(elm, elm2, field)                                     \
    do {                                                                   \
        if (((elm2)->field.le_next = (elm)->field.le_next) != NULL)        \
            (elm2)->field.le_next->field.le_prev = &(elm2)->field.le_next; \
        (elm2)->field.le_prev = (elm)->field.le_prev;                      \
        *(elm2)->field.le_prev = (elm2);                                   \
        _Q_INVALIDATE((elm)->field.le_prev);                               \
        _Q_INVALIDATE((elm)->field.le_next);                               \
    } while (0)

/******************************************************************************
 *                         Simple queue (SIMPLEQ)
 ******************************************************************************/
#define SIMPLEQ_HEAD(name, type)                            \
    struct name {                                           \
        struct type* sqh_first; /* first element */         \
        struct type** sqh_last; /* ptr to last next elem */ \
    }

#define SIMPLEQ_HEAD_INITIALIZER(head) {NULL, &(head).sqh_first}

#define SIMPLEQ_ENTRY(type)                    \
    struct {                                   \
        struct type* sqe_next; /* next elem */ \
    }

/* Access methods */
#define SIMPLEQ_FIRST(head) ((head)->sqh_first)
#define SIMPLEQ_END(head) NULL
#define SIMPLEQ_EMPTY(head) (SIMPLEQ_FIRST((head)) == SIMPLEQ_END(head))
#define SIMPLEQ_NEXT(elm, field) ((elm)->field.sqe_next)

/* Traversal */
#define SIMPLEQ_FOREACH(var, head, field) \
    for ((var) = SIMPLEQ_FIRST((head)); (var) != SIMPLEQ_END(head); (var) = SIMPLEQ_NEXT((var), field))

#define SIMPLEQ_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = SIMPLEQ_FIRST((head)); (var) && ((tvar) = SIMPLEQ_NEXT((var), field), 1); (var) = (tvar))

/* Functions */
#define SIMPLEQ_INIT(head)                     \
    do {                                       \
        (head)->sqh_first = NULL;              \
        (head)->sqh_last = &(head)->sqh_first; \
    } while (0)

#define SIMPLEQ_INSERT_HEAD(head, elm, field)                    \
    do {                                                         \
        if (((elm)->field.sqe_next = (head)->sqh_first) == NULL) \
            (head)->sqh_last = &(elm)->field.sqe_next;           \
        (head)->sqh_first = (elm);                               \
    } while (0)

#define SIMPLEQ_INSERT_TAIL(head, elm, field)      \
    do {                                           \
        (elm)->field.sqe_next = NULL;              \
        *(head)->sqh_last = (elm);                 \
        (head)->sqh_last = &(elm)->field.sqe_next; \
    } while (0)

#define SIMPLEQ_INSERT_AFTER(head, listelm, elm, field)                  \
    do {                                                                 \
        if (((elm)->field.sqe_next = (listelm)->field.sqe_next) == NULL) \
            (head)->sqh_last = &(elm)->field.sqe_next;                   \
        (listelm)->field.sqe_next = (elm);                               \
    } while (0)

#define SIMPLEQ_REMOVE_HEAD(head, field)                                     \
    do {                                                                     \
        if (((head)->sqh_first = (head)->sqh_first->field.sqe_next) == NULL) \
            (head)->sqh_last = &(head)->sqh_first;                           \
    } while (0)

#define SIMPLEQ_REMOVE_AFTER(head, elm, field)                                       \
    do {                                                                             \
        if (((elm)->field.sqe_next = (elm)->field.sqe_next->field.sqe_next) == NULL) \
            (head)->sqh_last = &(elm)->field.sqe_next;                               \
    } while (0)

#define SIMPLEQ_CONCAT(head1, head2)                 \
    do {                                             \
        if (!SIMPLEQ_EMPTY(head2)) {                 \
            *(head1)->sqh_last = (head2)->sqh_first; \
            (head1)->sqh_last = (head2)->sqh_last;   \
            SIMPLEQ_INIT((head2));                   \
        }                                            \
    } while (0)

/******************************************************************************
 *                 XOR Simple queue (XSIMPLEQ)
 *
 * Like SIMPLEQ but each pointer is XORed with a random cookie to help detect
 * certain pointer corruption. The cookie is generated by getrandom() in
 * XSIMPLEQ_INIT(). If your environment lacks getrandom(), modify queue_getrandom().
 ******************************************************************************/
#define XSIMPLEQ_HEAD(name, type)                     \
    struct name {                                     \
        struct type* sqx_first;   /* XORed ptr */     \
        struct type** sqx_last;   /* XORed ptr */     \
        unsigned long sqx_cookie; /* random cookie */ \
    }

#define XSIMPLEQ_ENTRY(type)                   \
    struct {                                   \
        struct type* sqx_next; /* XORed ptr */ \
    }

/* XOR encoding/decoding */
#define XSIMPLEQ_XOR(head, ptr) ((__typeof(ptr))((head)->sqx_cookie ^ (unsigned long)(ptr)))

/* Access methods */
#define XSIMPLEQ_FIRST(head) XSIMPLEQ_XOR((head), (head)->sqx_first)

#define XSIMPLEQ_END(head) NULL

#define XSIMPLEQ_EMPTY(head) (XSIMPLEQ_FIRST((head)) == XSIMPLEQ_END(head))

#define XSIMPLEQ_NEXT(head, elm, field) XSIMPLEQ_XOR((head), (elm)->field.sqx_next)

/* Traversal */
#define XSIMPLEQ_FOREACH(var, head, field) \
    for ((var) = XSIMPLEQ_FIRST(head); (var) != XSIMPLEQ_END(head); (var) = XSIMPLEQ_NEXT(head, var, field))

#define XSIMPLEQ_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = XSIMPLEQ_FIRST(head); (var) && ((tvar) = XSIMPLEQ_NEXT(head, var, field), 1); (var) = (tvar))

/* Functions */
static inline void XSIMPLEQ_INIT_impl(void** first, void*** last, unsigned long* cookie) {
    /* Fill cookie with random bytes. */
    queue_getrandom(cookie, sizeof(*cookie));

    /* XOR with NULL => just the cookie. */
    *first = (void*)(*cookie ^ (unsigned long)NULL);
    *last = (void**)(*cookie ^ (unsigned long)(&(*first)));
}

#define XSIMPLEQ_INIT(head)                                                                              \
    do {                                                                                                 \
        XSIMPLEQ_INIT_impl((void**)&(head)->sqx_first, (void***)&(head)->sqx_last, &(head)->sqx_cookie); \
    } while (0)

#define XSIMPLEQ_INSERT_HEAD(head, elm, field)                                       \
    do {                                                                             \
        if (((elm)->field.sqx_next = (head)->sqx_first) == XSIMPLEQ_XOR(head, NULL)) \
            (head)->sqx_last = XSIMPLEQ_XOR(head, &(elm)->field.sqx_next);           \
        (head)->sqx_first = XSIMPLEQ_XOR(head, (elm));                               \
    } while (0)

#define XSIMPLEQ_INSERT_TAIL(head, elm, field)                               \
    do {                                                                     \
        (elm)->field.sqx_next = XSIMPLEQ_XOR(head, NULL);                    \
        *(XSIMPLEQ_XOR(head, (head)->sqx_last)) = XSIMPLEQ_XOR(head, (elm)); \
        (head)->sqx_last = XSIMPLEQ_XOR(head, &(elm)->field.sqx_next);       \
    } while (0)

#define XSIMPLEQ_INSERT_AFTER(head, listelm, elm, field)                                     \
    do {                                                                                     \
        if (((elm)->field.sqx_next = (listelm)->field.sqx_next) == XSIMPLEQ_XOR(head, NULL)) \
            (head)->sqx_last = XSIMPLEQ_XOR(head, &(elm)->field.sqx_next);                   \
        (listelm)->field.sqx_next = XSIMPLEQ_XOR(head, (elm));                               \
    } while (0)

/*
 * The next two macros need an intermediate pointer to decode the current head,
 * so we do a small inline struct hack. The __xhead struct is just for clarity.
 */
struct __xhead {
    void* sqx_next;
};

#define XSIMPLEQ_REMOVE_HEAD(head, field)                                                        \
    do {                                                                                         \
        struct __xhead* tmp = (struct __xhead*)XSIMPLEQ_FIRST(head);                             \
        if (((head)->sqx_first = XSIMPLEQ_XOR(head, tmp->sqx_next)) == XSIMPLEQ_XOR(head, NULL)) \
            (head)->sqx_last = XSIMPLEQ_XOR(head, &(head)->sqx_first);                           \
    } while (0)

#define XSIMPLEQ_REMOVE_AFTER(head, elm, field)                                                      \
    do {                                                                                             \
        struct __xhead* tmp = (struct __xhead*)XSIMPLEQ_NEXT(head, elm, field);                      \
        if (((elm)->field.sqx_next = XSIMPLEQ_XOR(head, tmp->sqx_next)) == XSIMPLEQ_XOR(head, NULL)) \
            (head)->sqx_last = XSIMPLEQ_XOR(head, &(elm)->field.sqx_next);                           \
    } while (0)

/******************************************************************************
 *                         Tail queue (TAILQ)
 ******************************************************************************/
#define TAILQ_HEAD(name, type)                              \
    struct name {                                           \
        struct type* tqh_first; /* first element */         \
        struct type** tqh_last; /* ptr to last next elem */ \
    }

#define TAILQ_HEAD_INITIALIZER(head) {NULL, &(head).tqh_first}

#define TAILQ_ENTRY(type)                                  \
    struct {                                               \
        struct type* tqe_next;  /* next elem */            \
        struct type** tqe_prev; /* ptr to previous link */ \
    }

/* Access methods */
#define TAILQ_FIRST(head) ((head)->tqh_first)
#define TAILQ_END(head) NULL
#define TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)
#define TAILQ_EMPTY(head) (TAILQ_FIRST((head)) == TAILQ_END(head))

/* Reverse access (BSD extension) */
#define TAILQ_LAST(head, headname) (*(((struct headname*)((head)->tqh_last))->tqh_last))

#define TAILQ_PREV(elm, headname, field) (*(((struct headname*)((elm)->field.tqe_prev))->tqh_last))

/* Traversal */
#define TAILQ_FOREACH(var, head, field) \
    for ((var) = TAILQ_FIRST((head)); (var) != TAILQ_END(head); (var) = TAILQ_NEXT((var), field))

#define TAILQ_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = TAILQ_FIRST((head)); (var) && ((tvar) = TAILQ_NEXT((var), field), 1); (var) = (tvar))

#define TAILQ_FOREACH_REVERSE(var, head, headname, field) \
    for ((var) = TAILQ_LAST((head), headname); (var) != TAILQ_END(head); (var) = TAILQ_PREV((var), headname, field))

#define TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)                                      \
    for ((var) = TAILQ_LAST((head), headname); (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1); \
         (var) = (tvar))

/* Functions */
#define TAILQ_INIT(head)                       \
    do {                                       \
        (head)->tqh_first = NULL;              \
        (head)->tqh_last = &(head)->tqh_first; \
    } while (0)

#define TAILQ_INSERT_HEAD(head, elm, field)                             \
    do {                                                                \
        if (((elm)->field.tqe_next = (head)->tqh_first) != NULL)        \
            (head)->tqh_first->field.tqe_prev = &(elm)->field.tqe_next; \
        else                                                            \
            (head)->tqh_last = &(elm)->field.tqe_next;                  \
        (head)->tqh_first = (elm);                                      \
        (elm)->field.tqe_prev = &(head)->tqh_first;                     \
    } while (0)

#define TAILQ_INSERT_TAIL(head, elm, field)        \
    do {                                           \
        (elm)->field.tqe_next = NULL;              \
        (elm)->field.tqe_prev = (head)->tqh_last;  \
        *(head)->tqh_last = (elm);                 \
        (head)->tqh_last = &(elm)->field.tqe_next; \
    } while (0)

#define TAILQ_INSERT_AFTER(head, listelm, elm, field)                       \
    do {                                                                    \
        if (((elm)->field.tqe_next = (listelm)->field.tqe_next) != NULL)    \
            (elm)->field.tqe_next->field.tqe_prev = &(elm)->field.tqe_next; \
        else                                                                \
            (head)->tqh_last = &(elm)->field.tqe_next;                      \
        (listelm)->field.tqe_next = (elm);                                  \
        (elm)->field.tqe_prev = &(listelm)->field.tqe_next;                 \
    } while (0)

#define TAILQ_INSERT_BEFORE(listelm, elm, field)            \
    do {                                                    \
        (elm)->field.tqe_prev = (listelm)->field.tqe_prev;  \
        (elm)->field.tqe_next = (listelm);                  \
        *(listelm)->field.tqe_prev = (elm);                 \
        (listelm)->field.tqe_prev = &(elm)->field.tqe_next; \
    } while (0)

#define TAILQ_REMOVE(head, elm, field)                                     \
    do {                                                                   \
        if ((elm)->field.tqe_next != NULL)                                 \
            (elm)->field.tqe_next->field.tqe_prev = (elm)->field.tqe_prev; \
        else                                                               \
            (head)->tqh_last = (elm)->field.tqe_prev;                      \
        *(elm)->field.tqe_prev = (elm)->field.tqe_next;                    \
        _Q_INVALIDATE((elm)->field.tqe_prev);                              \
        _Q_INVALIDATE((elm)->field.tqe_next);                              \
    } while (0)

#define TAILQ_REPLACE(head, elm, elm2, field)                                 \
    do {                                                                      \
        if (((elm2)->field.tqe_next = (elm)->field.tqe_next) != NULL)         \
            (elm2)->field.tqe_next->field.tqe_prev = &(elm2)->field.tqe_next; \
        else                                                                  \
            (head)->tqh_last = &(elm2)->field.tqe_next;                       \
        (elm2)->field.tqe_prev = (elm)->field.tqe_prev;                       \
        *(elm2)->field.tqe_prev = (elm2);                                     \
        _Q_INVALIDATE((elm)->field.tqe_prev);                                 \
        _Q_INVALIDATE((elm)->field.tqe_next);                                 \
    } while (0)

#define TAILQ_CONCAT(head1, head2, field)                           \
    do {                                                            \
        if (!TAILQ_EMPTY(head2)) {                                  \
            *(head1)->tqh_last = (head2)->tqh_first;                \
            (head2)->tqh_first->field.tqe_prev = (head1)->tqh_last; \
            (head1)->tqh_last = (head2)->tqh_last;                  \
            TAILQ_INIT((head2));                                    \
        }                                                           \
    } while (0)

/******************************************************************************
 *                  Singly-linked Tail queue (STAILQ)
 ******************************************************************************/
#define STAILQ_HEAD(name, type)                              \
    struct name {                                            \
        struct type* stqh_first; /* first element */         \
        struct type** stqh_last; /* ptr to last next elem */ \
    }

#define STAILQ_HEAD_INITIALIZER(head) {NULL, &(head).stqh_first}

#define STAILQ_ENTRY(type)                      \
    struct {                                    \
        struct type* stqe_next; /* next elem */ \
    }

/* Access methods */
#define STAILQ_FIRST(head) ((head)->stqh_first)
#define STAILQ_END(head) NULL
#define STAILQ_EMPTY(head) (STAILQ_FIRST((head)) == STAILQ_END(head))
#define STAILQ_NEXT(elm, field) ((elm)->field.stqe_next)

/* Traversal */
#define STAILQ_FOREACH(var, head, field) \
    for ((var) = STAILQ_FIRST((head)); (var) != STAILQ_END(head); (var) = STAILQ_NEXT((var), field))

#define STAILQ_FOREACH_SAFE(var, head, field, tvar) \
    for ((var) = STAILQ_FIRST((head)); (var) && ((tvar) = STAILQ_NEXT((var), field), 1); (var) = (tvar))

/* Functions */
#define STAILQ_INIT(head)                          \
    do {                                           \
        STAILQ_FIRST((head)) = NULL;               \
        (head)->stqh_last = &STAILQ_FIRST((head)); \
    } while (0)

#define STAILQ_INSERT_HEAD(head, elm, field)                            \
    do {                                                                \
        if ((STAILQ_NEXT((elm), field) = STAILQ_FIRST((head))) == NULL) \
            (head)->stqh_last = &STAILQ_NEXT((elm), field);             \
        STAILQ_FIRST((head)) = (elm);                                   \
    } while (0)

#define STAILQ_INSERT_TAIL(head, elm, field)            \
    do {                                                \
        STAILQ_NEXT((elm), field) = NULL;               \
        *(head)->stqh_last = (elm);                     \
        (head)->stqh_last = &STAILQ_NEXT((elm), field); \
    } while (0)

#define STAILQ_INSERT_AFTER(head, listelm, elm, field)                           \
    do {                                                                         \
        if ((STAILQ_NEXT((elm), field) = STAILQ_NEXT((listelm), field)) == NULL) \
            (head)->stqh_last = &STAILQ_NEXT((elm), field);                      \
        STAILQ_NEXT((listelm), field) = (elm);                                   \
    } while (0)

#define STAILQ_REMOVE_HEAD(head, field)                                                \
    do {                                                                               \
        if ((STAILQ_FIRST((head)) = STAILQ_NEXT(STAILQ_FIRST((head)), field)) == NULL) \
            (head)->stqh_last = &STAILQ_FIRST((head));                                 \
    } while (0)

#define STAILQ_REMOVE_AFTER(head, elm, field)                                                    \
    do {                                                                                         \
        if ((STAILQ_NEXT((elm), field) = STAILQ_NEXT(STAILQ_NEXT((elm), field), field)) == NULL) \
            (head)->stqh_last = &STAILQ_NEXT((elm), field);                                      \
    } while (0)

#define STAILQ_REMOVE(head, elm, type, field)           \
    do {                                                \
        if (STAILQ_FIRST((head)) == (elm)) {            \
            STAILQ_REMOVE_HEAD((head), field);          \
        }                                               \
        else {                                          \
            struct type* curelm = (head)->stqh_first;   \
            while (STAILQ_NEXT(curelm, field) != (elm)) \
                curelm = STAILQ_NEXT(curelm, field);    \
            STAILQ_REMOVE_AFTER(head, curelm, field);   \
        }                                               \
    } while (0)

#define STAILQ_CONCAT(head1, head2)                    \
    do {                                               \
        if (!STAILQ_EMPTY((head2))) {                  \
            *(head1)->stqh_last = (head2)->stqh_first; \
            (head1)->stqh_last = (head2)->stqh_last;   \
            STAILQ_INIT((head2));                      \
        }                                              \
    } while (0)

#define STAILQ_LAST(head, type, field) \
    (STAILQ_EMPTY((head)) ? NULL : (struct type*)((char*)((head)->stqh_last) - offsetof(struct type, field)))

#endif /* !_SYS_QUEUE_H_ */
