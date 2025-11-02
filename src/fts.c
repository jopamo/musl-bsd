#define _XOPEN_SOURCE 700

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fts.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static inline int ISDOT(const char* a) {
    return (a[0] == '.' && (!a[1] || (a[1] == '.' && !a[2])));
}

static inline size_t fts_length_max(void) {
    return (size_t)((__fts_length_t)~(__fts_length_t)0);
}

static inline __fts_length_t fts_length_cap(size_t len) {
    const size_t max = fts_length_max();
    return len > max ? (__fts_length_t)max : (__fts_length_t)len;
}

#ifndef ALIGNBYTES
#define ALIGNBYTES (sizeof(long) - 1)
#endif
#ifndef ALIGN
#define ALIGN(p) (((uintptr_t)(p) + ALIGNBYTES) & ~((uintptr_t)ALIGNBYTES))
#endif

#define ISSET(opt) (sp->fts_options & (opt))
#define SET(opt) (sp->fts_options |= (opt))
#define CLR(opt) (sp->fts_options &= ~(opt))

#ifdef O_NOFOLLOW
#define HAS_O_NOFOLLOW 1
#else
#define HAS_O_NOFOLLOW 0
#warning "O_NOFOLLOW not supported – symlink race protection disabled"
#endif

#define BCHILD 1
#define BNAMES 2
#define BREAD 3

/* helpers */
static void* safe_recallocarray(void* ptr, size_t oldnmemb, size_t newnmemb, size_t size);
static FTSENT* fts_alloc(FTS*, const char*, size_t) __attribute__((nonnull));
static FTSENT* fts_build(FTS*, int);
static void fts_lfree(FTSENT*);
static void fts_load(FTS*, FTSENT*);
static size_t fts_maxarglen(char* const*);
static void fts_padjust(FTS*, FTSENT*);
static size_t fts_pow2(size_t);
static int fts_palloc(FTS*, size_t);
static FTSENT* fts_sort(FTS*, FTSENT*, int);
static unsigned short fts_stat(FTS*, FTSENT*, int, int);
static int fts_safe_changedir(FTS*, FTSENT*, int, const char*);

static void* safe_recallocarray(void* ptr, size_t oldnmemb, size_t newnmemb, size_t size) {
    if (size != 0 && newnmemb > SIZE_MAX / size) {
        errno = ENOMEM;
        return NULL;
    }
    const size_t oldsz = oldnmemb * size;
    const size_t newsz = newnmemb * size;
    if (newsz == 0) {
        free(ptr);
        return NULL;
    }
    void* ret = realloc(ptr, newsz);
    if (!ret)
        return NULL;
    if (newsz > oldsz)
        memset((char*)ret + oldsz, 0, newsz - oldsz);
    return ret;
}

FTS* fts_open(char* const* argv, int options, int (*compar)(const FTSENT**, const FTSENT**)) {
    FTS* sp;
    FTSENT* p;
    FTSENT* root = NULL;
    FTSENT* parent;
    FTSENT* prev = NULL;
    int nitems = 0;

    if ((options & ~FTS_OPTIONMASK) || argv == NULL) {
        errno = EINVAL;
        return NULL;
    }

    sp = calloc(1, sizeof(*sp));
    if (!sp)
        return NULL;

    sp->fts_compar = compar;
    sp->fts_options = options;

    if (ISSET(FTS_LOGICAL))
        SET(FTS_NOCHDIR);

    {
        size_t need = fts_maxarglen(argv);
        if (need < PATH_MAX)
            need = PATH_MAX;
        if (fts_palloc(sp, need)) {
            free(sp);
            return NULL;
        }
    }

    parent = fts_alloc(sp, "", 0);
    if (!parent) {
        free(sp->fts_path);
        free(sp);
        return NULL;
    }
    parent->fts_level = FTS_ROOTPARENTLEVEL;

    for (; *argv; ++argv, ++nitems) {
        size_t alen = strlen(*argv);
        if (alen == 0) {
            errno = ENOENT;
            fts_lfree(root);
            free(parent);
            free(sp->fts_path);
            free(sp);
            return NULL;
        }
        p = fts_alloc(sp, *argv, alen);
        if (!p) {
            fts_lfree(root);
            free(parent);
            free(sp->fts_path);
            free(sp);
            return NULL;
        }

        p->fts_level = FTS_ROOTLEVEL;
        p->fts_parent = parent;
        p->fts_accpath = p->fts_name;

        p->fts_info = fts_stat(sp, p, ISSET(FTS_COMFOLLOW), -1);
        if (p->fts_info == FTS_DOT)
            p->fts_info = FTS_D;

        if (compar) {
            p->fts_link = root;
            root = p;
        }
        else {
            p->fts_link = NULL;
            if (!root)
                root = p;
            else
                prev->fts_link = p;
            prev = p;
        }
    }

    if (compar && nitems > 1)
        root = fts_sort(sp, root, nitems);

    sp->fts_cur = fts_alloc(sp, "", 0);
    if (!sp->fts_cur)
        goto oom_roots;
    sp->fts_cur->fts_link = root;
    sp->fts_cur->fts_info = FTS_INIT;

    if (!ISSET(FTS_NOCHDIR)) {
        sp->fts_rfd = open(".", O_RDONLY | O_CLOEXEC);
        if (sp->fts_rfd == -1)
            SET(FTS_NOCHDIR);
    }

    if (nitems == 0)
        free(parent);
    return sp;

oom_roots:
    fts_lfree(root);
    free(parent);
    free(sp->fts_path);
    free(sp);
    return NULL;
}

int fts_close(FTS* sp) {
    int saved_errno = 0;

    if (!sp)
        return 0;

    if (sp->fts_cur) {
        FTSENT* p = sp->fts_cur;
        if (p->fts_flags & FTS_SYMFOLLOW)
            close(p->fts_symfd);
        while (p->fts_level >= FTS_ROOTLEVEL) {
            FTSENT* next = p->fts_link ? p->fts_link : p->fts_parent;
            free(p);
            p = next;
        }
        free(p);
    }

    if (sp->fts_child)
        fts_lfree(sp->fts_child);

    free(sp->fts_array);
    free(sp->fts_path);

    int rfd = ISSET(FTS_NOCHDIR) ? -1 : sp->fts_rfd;
    if (rfd != -1) {
        if (fchdir(rfd) == -1)
            saved_errno = errno;
        close(rfd);
    }

    free(sp);

    if (saved_errno) {
        errno = saved_errno;
        return -1;
    }

    return 0;
}

FTSENT* fts_read(FTS* sp) {
    FTSENT* p;
    FTSENT* tmp;
    int instr;
    char* t;
    int saved_errno;

    if (!sp->fts_cur || ISSET(FTS_STOP))
        return NULL;

    p = sp->fts_cur;
    instr = p->fts_instr;
    p->fts_instr = FTS_NOINSTR;

    if (instr == FTS_AGAIN) {
        p->fts_info = fts_stat(sp, p, 0, -1);
        return p;
    }

    if (instr == FTS_FOLLOW && (p->fts_info == FTS_SL || p->fts_info == FTS_SLNONE)) {
        p->fts_info = fts_stat(sp, p, 1, -1);
        if (p->fts_info == FTS_D && !ISSET(FTS_NOCHDIR)) {
            p->fts_symfd = open(".", O_RDONLY | O_CLOEXEC);
            if (p->fts_symfd == -1) {
                p->fts_errno = errno;
                p->fts_info = FTS_ERR;
            }
            else {
                p->fts_flags |= FTS_SYMFOLLOW;
            }
        }
        return p;
    }

    if (p->fts_info == FTS_D) {
        if (instr == FTS_SKIP || (ISSET(FTS_XDEV) && p->fts_dev != sp->fts_dev)) {
            if (p->fts_flags & FTS_SYMFOLLOW)
                close(p->fts_symfd);
            if (sp->fts_child) {
                fts_lfree(sp->fts_child);
                sp->fts_child = NULL;
            }
            p->fts_info = FTS_DP;
            return p;
        }

        if (sp->fts_child && ISSET(FTS_NAMEONLY)) {
            CLR(FTS_NAMEONLY);
            fts_lfree(sp->fts_child);
            sp->fts_child = NULL;
        }

        if (!sp->fts_child) {
            sp->fts_child = fts_build(sp, BREAD);
            if (!sp->fts_child) {
                if (ISSET(FTS_STOP))
                    return NULL;
                return p;
            }
        }
        else if (fts_safe_changedir(sp, p, -1, p->fts_accpath)) {
            p->fts_errno = errno;
            p->fts_flags |= FTS_DONTCHDIR;
            for (FTSENT* xx = sp->fts_child; xx; xx = xx->fts_link)
                xx->fts_accpath = xx->fts_parent->fts_accpath;
        }

        p = sp->fts_child;
        sp->fts_child = NULL;
        goto name;
    }

next:
    tmp = p;
    p = p->fts_link;

    if (p) {
        free(tmp);

        if (p->fts_level == FTS_ROOTLEVEL) {
            if (!ISSET(FTS_NOCHDIR) && fchdir(sp->fts_rfd)) {
                SET(FTS_STOP);
                return NULL;
            }
            fts_load(sp, p);
            sp->fts_cur = p;
            return p;
        }

        if (p->fts_instr == FTS_SKIP)
            goto next;
        if (p->fts_instr == FTS_FOLLOW) {
            p->fts_info = fts_stat(sp, p, 1, -1);
            if (p->fts_info == FTS_D && !ISSET(FTS_NOCHDIR)) {
                p->fts_symfd = open(".", O_RDONLY | O_CLOEXEC);
                if (p->fts_symfd == -1) {
                    p->fts_errno = errno;
                    p->fts_info = FTS_ERR;
                }
                else {
                    p->fts_flags |= FTS_SYMFOLLOW;
                }
            }
            p->fts_instr = FTS_NOINSTR;
        }

    name:
        t = sp->fts_path + ((p->fts_parent->fts_path[p->fts_parent->fts_pathlen - 1] == '/')
                                ? p->fts_parent->fts_pathlen - 1
                                : p->fts_parent->fts_pathlen);
        *t++ = '/';
        memmove(t, p->fts_name, p->fts_namelen + 1);
        sp->fts_cur = p;
        return p;
    }

    {
        FTSENT* up = tmp->fts_parent;
        free(tmp);
        p = up;
    }

    if (!p) {
        errno = EFAULT;
        return NULL;
    }

    sp->fts_path[p->fts_pathlen] = '\0';

    if (p->fts_level == FTS_ROOTPARENTLEVEL) {
        free(p);
        errno = 0;
        sp->fts_cur = NULL;
        return NULL;
    }

    if (p->fts_level == FTS_ROOTLEVEL) {
        if (!ISSET(FTS_NOCHDIR) && fchdir(sp->fts_rfd)) {
            SET(FTS_STOP);
            sp->fts_cur = p;
            return NULL;
        }
    }
    else if (p->fts_flags & FTS_SYMFOLLOW) {
        if (!ISSET(FTS_NOCHDIR) && fchdir(p->fts_symfd)) {
            saved_errno = errno;
            close(p->fts_symfd);
            errno = saved_errno;
            SET(FTS_STOP);
            sp->fts_cur = p;
            return NULL;
        }
        close(p->fts_symfd);
    }
    else if (!(p->fts_flags & FTS_DONTCHDIR) && fts_safe_changedir(sp, p->fts_parent, -1, "..")) {
        SET(FTS_STOP);
        sp->fts_cur = p;
        return NULL;
    }

    p->fts_info = p->fts_errno ? FTS_ERR : FTS_DP;
    sp->fts_cur = p;
    return p;
}

int fts_set(FTS* sp, FTSENT* p, int instr) {
    (void)sp;
    if (instr && instr != FTS_AGAIN && instr != FTS_FOLLOW && instr != FTS_SKIP && instr != FTS_NOINSTR) {
        errno = EINVAL;
        return 1;
    }
    p->fts_instr = instr;
    return 0;
}

FTSENT* fts_children(FTS* sp, int instr) {
    if (instr && instr != FTS_NAMEONLY) {
        errno = EINVAL;
        return NULL;
    }
    FTSENT* cur = sp->fts_cur;
    if (!cur || ISSET(FTS_STOP))
        return NULL;
    if (cur->fts_info == FTS_INIT)
        return cur->fts_link;
    if (cur->fts_info != FTS_D)
        return NULL;

    if (sp->fts_child)
        fts_lfree(sp->fts_child);

    if (instr == FTS_NAMEONLY)
        SET(FTS_NAMEONLY);

    if (cur->fts_level == FTS_ROOTLEVEL && cur->fts_accpath[0] != '/' && !ISSET(FTS_NOCHDIR)) {
        int cwd = open(".", O_RDONLY | O_CLOEXEC);
        if (cwd == -1)
            return NULL;
        sp->fts_child = fts_build(sp, instr == FTS_NAMEONLY ? BNAMES : BCHILD);
        if (fchdir(cwd) == -1) {
            close(cwd);
            return NULL;
        }
        close(cwd);
    }
    else {
        sp->fts_child = fts_build(sp, instr == FTS_NAMEONLY ? BNAMES : BCHILD);
    }
    return sp->fts_child;
}

static FTSENT* fts_build(FTS* sp, int type) {
    FTSENT* head = NULL;
    FTSENT* tail = NULL;
    FTSENT* cur = sp->fts_cur;
    FTSENT* p;
    DIR* dirp = NULL;
    struct dirent* dp;
    size_t len, maxlen;
    int nitems = 0;
    int cderrno = 0;
    int descend = 0;
    int level;
    int nlinks, nostat;
    int saved_errno;
    char* cp;

    if (cur->fts_level >= SHRT_MAX) {
        errno = ENAMETOOLONG;
        cur->fts_info = FTS_ERR;
        SET(FTS_STOP);
        return NULL;
    }

    int open_flags = O_RDONLY | O_DIRECTORY | O_CLOEXEC;
#if HAS_O_NOFOLLOW
    if (ISSET(FTS_PHYSICAL))
        open_flags |= O_NOFOLLOW;
#endif
    int fd = open(cur->fts_accpath, open_flags);
    if (fd == -1) {
        cur->fts_info = (type == BREAD) ? FTS_DNR : FTS_ERR;
        cur->fts_errno = errno;
        return NULL;
    }

    dirp = fdopendir(fd);
    if (!dirp) {
        saved_errno = errno;
        close(fd);
        cur->fts_info = FTS_ERR;
        cur->fts_errno = saved_errno;
        errno = saved_errno;
        return NULL;
    }

    if (type == BNAMES) {
        nlinks = 0;
        nostat = 1;
    }
    else if (ISSET(FTS_NOSTAT) && ISSET(FTS_PHYSICAL)) {
        nlinks = (int)cur->fts_nlink - (ISSET(FTS_SEEDOT) ? 0 : 2);
        if (nlinks < 0)
            nlinks = 0;
        nostat = 1;
    }
    else {
        nlinks = -1;
        nostat = 0;
    }

#ifndef DT_DIR
    (void)nostat;
#endif

    if ((nlinks != 0) || (type == BREAD)) {
        if (cur->fts_level == FTS_ROOTLEVEL) {
            cur->fts_flags |= FTS_DONTCHDIR;
        }
        else if (fts_safe_changedir(sp, cur, dirfd(dirp), NULL)) {
            cderrno = errno;
            cur->fts_flags |= FTS_DONTCHDIR;
        }
        else {
            descend = 1;
        }
    }

    len = ((cur->fts_path[cur->fts_pathlen - 1] == '/') ? cur->fts_pathlen - 1 : cur->fts_pathlen);
    if (ISSET(FTS_NOCHDIR)) {
        cp = sp->fts_path + len;
        *cp++ = '/';
    }
    len++;
    maxlen = sp->fts_pathlen - len;

    level = (cur->fts_level < SHRT_MAX) ? cur->fts_level + 1 : SHRT_MAX;

    errno = 0;
    while ((dp = readdir(dirp)) != NULL) {
        if (!ISSET(FTS_SEEDOT) && ISDOT(dp->d_name))
            continue;

        size_t dnamlen = strlen(dp->d_name);

        p = fts_alloc(sp, dp->d_name, dnamlen);
        if (!p)
            goto mem_fail;

        if (dnamlen >= maxlen) {
            char* oldaddr = sp->fts_path;
            if (fts_palloc(sp, dnamlen + len + 1))
                goto mem_fail;
            if (oldaddr != sp->fts_path) {
                fts_padjust(sp, head);
                if (ISSET(FTS_NOCHDIR))
                    cp = sp->fts_path + (len - 1);
            }
            maxlen = sp->fts_pathlen - len;
        }

        p->fts_level = level;
        p->fts_parent = cur;
        size_t pathlen = len + dnamlen;
        if (pathlen < len || pathlen > fts_length_max()) {
            free(p);
            errno = ENAMETOOLONG;
            goto mem_fail;
        }
        p->fts_pathlen = fts_length_cap(pathlen);

        if (cderrno) {
            p->fts_info = nlinks ? FTS_NS : FTS_NSOK;
            p->fts_errno = cderrno;
            p->fts_accpath = cur->fts_accpath;
        }
        else if (nlinks == 0
#ifdef DT_DIR
                 || (nostat && dp->d_type != DT_DIR && dp->d_type != DT_UNKNOWN)
#endif
        ) {
            p->fts_info = FTS_NSOK;
            p->fts_accpath = ISSET(FTS_NOCHDIR) ? p->fts_path : p->fts_name;
        }
        else {
            if (ISSET(FTS_NOCHDIR)) {
                p->fts_accpath = p->fts_path;
                memcpy(cp, p->fts_name, p->fts_namelen + 1);
                p->fts_info = fts_stat(sp, p, 0, dirfd(dirp));
            }
            else {
                p->fts_accpath = p->fts_name;
                p->fts_info = fts_stat(sp, p, 0, -1);
            }
            if (nlinks > 0 && (p->fts_info == FTS_D || p->fts_info == FTS_DC || p->fts_info == FTS_DOT))
                --nlinks;
        }

        p->fts_link = NULL;
        if (!head)
            head = tail = p;
        else {
            tail->fts_link = p;
            tail = p;
        }
        ++nitems;
        continue;

    mem_fail:
        saved_errno = errno;
        fts_lfree(head);
        closedir(dirp);
        cur->fts_info = FTS_ERR;
        SET(FTS_STOP);
        errno = saved_errno;
        return NULL;
    }

    if (errno != 0) {
        saved_errno = errno;
        fts_lfree(head);
        closedir(dirp);
        cur->fts_info = FTS_ERR;
        SET(FTS_STOP);
        errno = saved_errno;
        return NULL;
    }

    closedir(dirp);

    if (nitems == 0) {
        if (type == BREAD)
            cur->fts_info = FTS_DP;
        return NULL;
    }

    if (sp->fts_compar && nitems > 1)
        head = fts_sort(sp, head, nitems);

    if (descend && (type == BCHILD || nitems == 0)) {
        if (cur->fts_level == FTS_ROOTLEVEL) {
            if (fchdir(sp->fts_rfd) == -1) {
                cur->fts_info = FTS_ERR;
                SET(FTS_STOP);
                return NULL;
            }
        }
        else if (fts_safe_changedir(sp, cur->fts_parent, -1, "..")) {
            cur->fts_info = FTS_ERR;
            SET(FTS_STOP);
            return NULL;
        }
    }

    return head;
}

static unsigned short fts_stat(FTS* sp, FTSENT* p, int follow, int dfd) {
    __fts_stat_t sb;
    __fts_stat_t* sbp;
    const char* path;
    int saved_errno;

    if (p->fts_level >= SHRT_MAX) {
        errno = ERANGE;
        p->fts_errno = ERANGE;
        return FTS_ERR;
    }

    if (dfd == -1) {
        path = p->fts_accpath;
        dfd = AT_FDCWD;
    }
    else {
        path = p->fts_name;
    }

    sbp = ISSET(FTS_NOSTAT) ? &sb : p->fts_statp;

    if (ISSET(FTS_LOGICAL) || follow) {
        if (fstatat(dfd, path, sbp, 0) == -1) {
            saved_errno = errno;
            if (fstatat(dfd, path, sbp, AT_SYMLINK_NOFOLLOW) == 0) {
                errno = 0;
                return FTS_SLNONE;
            }
            p->fts_errno = saved_errno;
            goto err;
        }
    }
    else {
        if (fstatat(dfd, path, sbp, AT_SYMLINK_NOFOLLOW) == -1) {
            p->fts_errno = errno;
            goto err;
        }
    }

    if (S_ISDIR(sbp->st_mode)) {
        p->fts_dev = sbp->st_dev;
        p->fts_ino = sbp->st_ino;
        p->fts_nlink = sbp->st_nlink;

        if (ISDOT(p->fts_name))
            return FTS_DOT;

        for (FTSENT* t = p->fts_parent; t->fts_level >= FTS_ROOTLEVEL; t = t->fts_parent) {
            if (p->fts_ino == t->fts_ino && p->fts_dev == t->fts_dev) {
                p->fts_cycle = t;
                return FTS_DC;
            }
        }
        return FTS_D;
    }
    if (S_ISLNK(sbp->st_mode))
        return FTS_SL;
    if (S_ISREG(sbp->st_mode))
        return FTS_F;

    return FTS_DEFAULT;

err:
    memset(sbp, 0, sizeof(__fts_stat_t));
    return FTS_NS;
}

static FTSENT* fts_sort(FTS* sp, FTSENT* head, int nitems) {
    if ((unsigned int)nitems > sp->fts_nitems) {
        FTSENT** a = safe_recallocarray(sp->fts_array, sp->fts_nitems, nitems + 40, sizeof(FTSENT*));
        if (!a) {
            free(sp->fts_array);
            sp->fts_array = NULL;
            sp->fts_nitems = 0;
            return head;
        }
        sp->fts_nitems = fts_length_cap((size_t)nitems + 40);
        sp->fts_array = a;
    }
    FTSENT** ap = sp->fts_array;
    int i;
    for (FTSENT* p = head; p; p = p->fts_link)
        *ap++ = p;

    qsort(sp->fts_array, (size_t)nitems, sizeof(FTSENT*), (int (*)(const void*, const void*))sp->fts_compar);

    FTSENT* newhead = sp->fts_array[0];
    for (i = 0; i < nitems - 1; i++)
        sp->fts_array[i]->fts_link = sp->fts_array[i + 1];
    sp->fts_array[nitems - 1]->fts_link = NULL;

    return newhead;
}

static FTSENT* fts_alloc(FTS* sp, const char* name, size_t namelen) {
    size_t len = sizeof(FTSENT) + namelen + 1;
    if (!ISSET(FTS_NOSTAT))
        len += sizeof(__fts_stat_t) + ALIGNBYTES;

    FTSENT* p = calloc(1, len);
    if (!p)
        return NULL;

    p->fts_path = sp->fts_path;
    p->fts_namelen = fts_length_cap(namelen);
    p->fts_instr = FTS_NOINSTR;

    if (!ISSET(FTS_NOSTAT)) {
        uintptr_t base = (uintptr_t)&p->fts_name[namelen + 1];
        base = ALIGN(base);
        p->fts_statp = (__fts_stat_t*)base;
    }

    memcpy(p->fts_name, name, namelen);
    p->fts_name[namelen] = '\0';

    return p;
}

static void fts_lfree(FTSENT* head) {
    while (head) {
        FTSENT* next = head->fts_link;
        free(head);
        head = next;
    }
}

static size_t fts_pow2(size_t x) {
    if (x == 0)
        return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
#if ULONG_MAX > 0xffffffffUL
    x |= x >> 32;
#endif
    x++;
    return x;
}

static int fts_palloc(FTS* sp, size_t need) {
    const size_t pad = 256;

    if (need > SIZE_MAX - pad) {
        errno = ENAMETOOLONG;
        return 1;
    }
    need += pad;
    need = fts_pow2(need);

    char* newbuf = realloc(sp->fts_path, need);
    if (!newbuf)
        return 1;

    sp->fts_path = newbuf;
    sp->fts_pathlen = fts_length_cap(need);
    return 0;
}

static void fts_padjust(FTS* sp, FTSENT* head) {
    char* addr = sp->fts_path;
#define ADJUST(e)                                                      \
    do {                                                               \
        if ((e)->fts_accpath != (e)->fts_name) {                       \
            size_t delta = (size_t)((e)->fts_accpath - (e)->fts_path); \
            (e)->fts_accpath = addr + delta;                           \
        }                                                              \
        (e)->fts_path = addr;                                          \
    } while (0)

    for (FTSENT* p = sp->fts_child; p; p = p->fts_link)
        ADJUST(p);

    for (FTSENT* p = head; p && p->fts_level >= FTS_ROOTLEVEL;) {
        ADJUST(p);
        p = p->fts_link ? p->fts_link : p->fts_parent;
    }
#undef ADJUST
}

static size_t fts_maxarglen(char* const* argv) {
    size_t max = 0;
    for (; *argv; ++argv) {
        size_t len = strlen(*argv);
        if (len > max)
            max = len;
    }
    return max + 1;
}

static int fts_safe_changedir(FTS* sp, FTSENT* p, int fd, const char* path) {
    if (ISSET(FTS_NOCHDIR))
        return 0;

    int newfd = fd;
    if (fd == -1) {
        int oflags = O_RDONLY | O_DIRECTORY | O_CLOEXEC
#if HAS_O_NOFOLLOW
                     | O_NOFOLLOW
#endif
            ;
        newfd = open(path ? path : p->fts_accpath, oflags);
        if (newfd == -1)
            return -1;
    }

    struct stat before;
    if (fstat(newfd, &before) == -1) {
        int e = errno;
        if (fd == -1)
            close(newfd);
        errno = e;
        return -1;
    }

    if (p->fts_dev != before.st_dev || p->fts_ino != before.st_ino) {
        if (fd == -1)
            close(newfd);
        errno = ENOENT;
        return -1;
    }

    if (fchdir(newfd) == -1) {
        int e = errno;
        if (fd == -1)
            close(newfd);
        errno = e;
        return -1;
    }

    struct stat after;
    if (fstat(newfd, &after) == -1 || before.st_dev != after.st_dev || before.st_ino != after.st_ino) {
        if (fd == -1)
            close(newfd);
        errno = ENOENT;
        return -1;
    }

    if (fd == -1)
        close(newfd);
    return 0;
}

static void fts_load(FTS* sp, FTSENT* p) {
    size_t len = p->fts_namelen;
    p->fts_pathlen = p->fts_namelen;
    memmove(sp->fts_path, p->fts_name, len + 1);

    char* slash = strrchr(p->fts_name, '/');
    if (slash && (slash != p->fts_name || slash[1])) {
        len = strlen(++slash);
        memmove(p->fts_name, slash, len + 1);
        p->fts_namelen = fts_length_cap(len);
    }

    p->fts_path = sp->fts_path;
    p->fts_accpath = sp->fts_path;
    p->fts_pathlen = fts_length_cap(len);
    sp->fts_dev = p->fts_dev;
}
