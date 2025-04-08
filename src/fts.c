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

#ifdef __cplusplus
extern "C" {
#endif

#define FTS_ROOTPARENTLEVEL -1
#define FTS_ROOTLEVEL 0
#define FTS_MAXLEVEL 0x7fffffff

#define FTS_D 1
#define FTS_DC 2
#define FTS_DEFAULT 3
#define FTS_DNR 4
#define FTS_ERR 5
#define FTS_F 6
#define FTS_INIT 7
#define FTS_NS 8
#define FTS_NSOK 9
#define FTS_SL 10
#define FTS_SLNONE 11
#define FTS_DP 12
#define FTS_DOT 13

#define FTS_AGAIN 1
#define FTS_FOLLOW 2
#define FTS_NOINSTR 3
#define FTS_SKIP 4

#define FTS_NAMEONLY 1

#define FTS_COMFOLLOW 0x0001
#define FTS_LOGICAL 0x0002
#define FTS_NOCHDIR 0x0004
#define FTS_NOSTAT 0x0008
#define FTS_PHYSICAL 0x0010
#define FTS_SEEDOT 0x0020
#define FTS_XDEV 0x0040
#define FTS_STOP 0x2000

#define FTS_OPTIONMASK (FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR | FTS_NOSTAT | FTS_PHYSICAL | FTS_SEEDOT | FTS_XDEV)

#define FTS_SYMFOLLOW 0x01
#define FTS_DONTCHDIR 0x02

typedef struct _ftsent {
    struct _ftsent* fts_cycle;
    struct _ftsent* fts_parent;
    struct _ftsent* fts_link;
    struct stat* fts_statp;
    char* fts_accpath;
    char* fts_path;
    off_t fts_nlink;
    dev_t fts_dev;
    ino_t fts_ino;
    int fts_symfd;
    short fts_info;
    short fts_flags;
    short fts_instr;
    short fts_level;
    int fts_errno;
    size_t fts_pathlen;
    size_t fts_namelen;
    char fts_name[];
} FTSENT;

typedef struct _fts {
    FTSENT* fts_cur;
    FTSENT* fts_child;
    FTSENT** fts_array;
    int fts_nitems;
    char* fts_path;
    size_t fts_pathlen;
    int fts_rfd;
    int fts_options;
    dev_t fts_dev;
    int (*fts_compar)(const FTSENT**, const FTSENT**);
} FTS;

FTS* fts_open(char* const* argv, int options, int (*compar)(const FTSENT**, const FTSENT**));
FTSENT* fts_read(FTS*);
FTSENT* fts_children(FTS*, int);
int fts_close(FTS*);
int fts_set(FTS*, FTSENT*, int);

#ifndef ALIGNBYTES
#define ALIGNBYTES (sizeof(long) - 1)
#endif
#ifndef ALIGN
#define ALIGN(p) (((uintptr_t)(p) + ALIGNBYTES) & ~((uintptr_t)ALIGNBYTES))
#endif

static inline int ISDOT(const char* a) {
    return (a[0] == '.' && (!a[1] || (a[1] == '.' && !a[2])));
}

#define ISSET(opt) (sp->fts_options & (opt))
#define SET(opt) (sp->fts_options |= (opt))
#define CLR(opt) (sp->fts_options &= ~(opt))

static void* safe_recallocarray(void* ptr, size_t oldnmemb, size_t newnmemb, size_t size) {
    if (newnmemb && size && newnmemb > (SIZE_MAX / size)) {
        errno = ENOMEM;
        return NULL;
    }
    size_t oldsz = oldnmemb * size;
    size_t newsz = newnmemb * size;
    void* ret = realloc(ptr, newsz);
    if (ret && newsz > oldsz) {
        memset((char*)ret + oldsz, 0, newsz - oldsz);
    }
    return ret;
}

static FTSENT* fts_alloc(FTS*, const char*, size_t);
static FTSENT* fts_build(FTS*, int);
static void fts_lfree(FTSENT*);
static void fts_load(FTS*, FTSENT*);
static size_t fts_maxarglen(char* const*);
static void fts_padjust(FTS*, FTSENT*);
static int fts_palloc(FTS*, size_t);
static FTSENT* fts_sort(FTS*, FTSENT*, int);
static unsigned short fts_stat(FTS*, FTSENT*, int, int);
static int fts_safe_changedir(FTS*, FTSENT*, int, const char*);

#define BCHILD 1
#define BNAMES 2
#define BREAD 3

#define NAPPEND(p) ((p)->fts_path[(p)->fts_pathlen - 1] == '/' ? (p)->fts_pathlen - 1 : (p)->fts_pathlen)

FTS* fts_open(char* const* argv, int options, int (*compar)(const FTSENT**, const FTSENT**)) {
    FTS* sp;
    FTSENT *p, *root;
    int nitems;
    FTSENT *parent, *prev;

    if ((options & ~FTS_OPTIONMASK) || (argv == NULL) || (*argv == NULL)) {
        errno = EINVAL;
        return NULL;
    }

    sp = calloc(1, sizeof(FTS));
    if (sp == NULL)
        return NULL;

    sp->fts_compar = compar;
    sp->fts_options = options;

    if (ISSET(FTS_LOGICAL))
        SET(FTS_NOCHDIR);

    {
        size_t need = fts_maxarglen(argv);
        if (need < PATH_MAX) {
            need = PATH_MAX;
        }
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

    for (root = prev = NULL, nitems = 0; *argv; argv++, nitems++) {
        size_t alen = strlen(*argv);
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
            if (root == NULL)
                root = p;
            else
                prev->fts_link = p;
            prev = p;
        }
    }

    if (compar && nitems > 1)
        root = fts_sort(sp, root, nitems);

    sp->fts_cur = fts_alloc(sp, "", 0);
    if (!sp->fts_cur) {
        fts_lfree(root);
        free(parent);
        free(sp->fts_path);
        free(sp);
        return NULL;
    }
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
}

int fts_close(FTS* sp) {
    FTSENT *freep, *p;
    int rfd, error = 0;

    if (!sp)
        return 0;

    if (sp->fts_cur) {
        for (p = sp->fts_cur; p->fts_level >= FTS_ROOTLEVEL;) {
            freep = p;
            p = p->fts_link ? p->fts_link : p->fts_parent;
            free(freep);
        }

        free(p);
    }

    rfd = ISSET(FTS_NOCHDIR) ? -1 : sp->fts_rfd;

    if (sp->fts_child)
        fts_lfree(sp->fts_child);

    free(sp->fts_array);
    free(sp->fts_path);

    free(sp);

    if (rfd != -1) {
        int saved_errno;
        if (fchdir(rfd) == -1)
            error = -1;
        saved_errno = errno;
        close(rfd);
        errno = saved_errno;
    }

    return error;
}

FTSENT* fts_read(FTS* sp) {
    FTSENT *p, *tmp;
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
        else {
            if (fts_safe_changedir(sp, p, -1, p->fts_accpath)) {
                p->fts_errno = errno;
                p->fts_flags |= FTS_DONTCHDIR;
                for (FTSENT* xx = sp->fts_child; xx; xx = xx->fts_link)
                    xx->fts_accpath = xx->fts_parent->fts_accpath;
            }
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
            if (!(ISSET(FTS_NOCHDIR)) && fchdir(sp->fts_rfd)) {
                SET(FTS_STOP);
                return NULL;
            }
            fts_load(sp, p);
            return (sp->fts_cur = p);
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
        t = sp->fts_path + NAPPEND(p->fts_parent);
        *t++ = '/';
        memmove(t, p->fts_name, p->fts_namelen + 1);
        return (sp->fts_cur = p);
    }

    p = tmp->fts_parent;
    free(tmp);

    if (p->fts_level == FTS_ROOTPARENTLEVEL) {
        free(p);
        errno = 0;
        return (sp->fts_cur = NULL);
    }

    sp->fts_path[p->fts_pathlen] = '\0';

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
    return (sp->fts_cur = p);
}

int fts_set(FTS* sp, FTSENT* p, int instr) {
    (void)sp;

    if (instr != 0 && instr != FTS_AGAIN && instr != FTS_FOLLOW && instr != FTS_NOINSTR && instr != FTS_SKIP) {
        errno = EINVAL;
        return 1;
    }
    p->fts_instr = instr;
    return 0;
}

FTSENT* fts_children(FTS* sp, int instr) {
    FTSENT* p;
    int fd;

    if (instr != 0 && instr != FTS_NAMEONLY) {
        errno = EINVAL;
        return NULL;
    }
    p = sp->fts_cur;
    if (!p || ISSET(FTS_STOP))
        return NULL;
    if (p->fts_info == FTS_INIT)
        return p->fts_link;
    if (p->fts_info != FTS_D)
        return NULL;

    if (sp->fts_child)
        fts_lfree(sp->fts_child);

    if (instr == FTS_NAMEONLY) {
        SET(FTS_NAMEONLY);
    }

    if ((p->fts_level == FTS_ROOTLEVEL) && p->fts_accpath[0] != '/' && !ISSET(FTS_NOCHDIR)) {
        fd = open(".", O_RDONLY | O_CLOEXEC);
        if (fd == -1)
            return NULL;
        sp->fts_child = fts_build(sp, instr == FTS_NAMEONLY ? BNAMES : BCHILD);
        if (fchdir(fd)) {
            close(fd);
            return NULL;
        }
        close(fd);
        return sp->fts_child;
    }

    sp->fts_child = fts_build(sp, instr == FTS_NAMEONLY ? BNAMES : BCHILD);
    return sp->fts_child;
}

static FTSENT* fts_build(FTS* sp, int type) {
    DIR* dirp = NULL;
    struct dirent* dp;
    FTSENT *p, *head, *tail, *cur;
    size_t len, maxlen;
    int nitems = 0;
    int cderrno = 0, descend = 0, level;
    int nlinks, nostat;
    int saved_errno;
    char* cp;

    cur = sp->fts_cur;
    dirp = opendir(cur->fts_accpath);
    if (!dirp) {
        if (type == BREAD) {
            cur->fts_info = FTS_DNR;
            cur->fts_errno = errno;
        }
        return NULL;
    }

    if (type == BNAMES) {
        nlinks = 0;
        nostat = 1;
    }
    else if (ISSET(FTS_NOSTAT) && ISSET(FTS_PHYSICAL)) {
        nlinks = (int)(cur->fts_nlink) - (ISSET(FTS_SEEDOT) ? 0 : 2);
        nostat = 1;
    }
    else {
        nlinks = -1;
        nostat = 0;
    }

    if ((nlinks != 0) || (type == BREAD)) {
        if (cur->fts_level == FTS_ROOTLEVEL) {
            descend = 0;
            cderrno = 0;
            cur->fts_flags |= FTS_DONTCHDIR;
        }
        else if (fts_safe_changedir(sp, cur, dirfd(dirp), NULL)) {
            if (nlinks && type == BREAD)
                cur->fts_errno = errno;
            cur->fts_flags |= FTS_DONTCHDIR;
            descend = 0;
            cderrno = errno;
            closedir(dirp);
            dirp = NULL;
        }
        else {
            descend = 1;
        }
    }
    else {
        descend = 0;
    }

    len = NAPPEND(cur);
    if (ISSET(FTS_NOCHDIR)) {
        cp = sp->fts_path + len;
        *cp++ = '/';
    }
    len++;
    maxlen = sp->fts_pathlen - len;

    level = cur->fts_level;
    if (level < FTS_MAXLEVEL)
        level++;

    int doadjust = 0;
    head = tail = NULL;

    if (dirp) {
        while ((dp = readdir(dirp))) {
            if (!ISSET(FTS_SEEDOT) && ISDOT(dp->d_name))
                continue;

            size_t d_namlen = strlen(dp->d_name);

            p = fts_alloc(sp, dp->d_name, d_namlen);
            if (!p) {
            mem1:
                saved_errno = errno;
                free(p);
                fts_lfree(head);
                closedir(dirp);
                cur->fts_info = FTS_ERR;
                SET(FTS_STOP);
                errno = saved_errno;
                return NULL;
            }

            if (d_namlen >= maxlen) {
                void* oldaddr = sp->fts_path;
                if (fts_palloc(sp, d_namlen + len + 1))
                    goto mem1;

                if (oldaddr != sp->fts_path) {
                    doadjust = 1;
                    if (ISSET(FTS_NOCHDIR))
                        cp = sp->fts_path + (len - 1);
                }
                maxlen = sp->fts_pathlen - len;
            }

            p->fts_level = level;
            p->fts_parent = cur;
            p->fts_pathlen = len + d_namlen;
            if (p->fts_pathlen < len) {
                free(p);
                fts_lfree(head);
                closedir(dirp);
                cur->fts_info = FTS_ERR;
                SET(FTS_STOP);
                errno = ENAMETOOLONG;
                return NULL;
            }

            if (cderrno) {
                if (nlinks)
                    p->fts_info = FTS_NS;
                else
                    p->fts_info = FTS_NSOK;
                p->fts_errno = cderrno;
                p->fts_accpath = cur->fts_accpath;
            }
            else if (nlinks == 0
#ifdef DT_DIR

                     || (nostat && dp->d_type != DT_DIR && dp->d_type != DT_UNKNOWN)
#endif
            ) {
                p->fts_info = FTS_NSOK;
                p->fts_accpath = (ISSET(FTS_NOCHDIR) ? p->fts_path : p->fts_name);
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
            if (!head) {
                head = tail = p;
            }
            else {
                tail->fts_link = p;
                tail = p;
            }
            nitems++;
        }
        closedir(dirp);
    }

    if (doadjust)
        fts_padjust(sp, head);

    if (ISSET(FTS_NOCHDIR)) {
        if (nitems == 0) {
            cp = sp->fts_path + (len - 1);
        }
        *cp = '\0';
    }

    if (descend && ((type == BCHILD) || (nitems == 0))) {
        if (cur->fts_level == FTS_ROOTLEVEL) {
            if (fchdir(sp->fts_rfd)) {
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

    if (!nitems) {
        if (type == BREAD)
            cur->fts_info = FTS_DP;
        return NULL;
    }

    if (sp->fts_compar && nitems > 1)
        head = fts_sort(sp, head, nitems);

    return head;
}

static unsigned short fts_stat(FTS* sp, FTSENT* p, int follow, int dfd) {
    struct stat *sbp, sb;
    int saved_errno;
    const char* path;

    if (dfd == -1) {
        path = p->fts_accpath;
        dfd = AT_FDCWD;
    }
    else {
        path = p->fts_name;
    }

    sbp = (ISSET(FTS_NOSTAT)) ? &sb : p->fts_statp;

    if (ISSET(FTS_LOGICAL) || follow) {
        if (fstatat(dfd, path, sbp, 0)) {
            saved_errno = errno;
            if (!fstatat(dfd, path, sbp, AT_SYMLINK_NOFOLLOW)) {
                errno = 0;
                return FTS_SLNONE;
            }
            p->fts_errno = saved_errno;
            goto err;
        }
    }
    else {
        if (fstatat(dfd, path, sbp, AT_SYMLINK_NOFOLLOW)) {
            p->fts_errno = errno;
        err:
            memset(sbp, 0, sizeof(struct stat));
            return FTS_NS;
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
}

static FTSENT* fts_sort(FTS* sp, FTSENT* head, int nitems) {
    if (nitems > sp->fts_nitems) {
        FTSENT** a;

        a = safe_recallocarray(sp->fts_array, sp->fts_nitems, nitems + 40, sizeof(FTSENT*));
        if (!a) {
            free(sp->fts_array);
            sp->fts_array = NULL;
            sp->fts_nitems = 0;
            return head;
        }
        sp->fts_nitems = nitems + 40;
        sp->fts_array = a;
    }

    FTSENT** ap = sp->fts_array;
    for (FTSENT* p = head; p; p = p->fts_link)
        *ap++ = p;

    qsort(sp->fts_array, nitems, sizeof(FTSENT*), (int (*)(const void*, const void*))sp->fts_compar);

    FTSENT* newhead = sp->fts_array[0];
    ap = sp->fts_array;
    for (int i = 0; i < (nitems - 1); i++)
        sp->fts_array[i]->fts_link = sp->fts_array[i + 1];
    sp->fts_array[nitems - 1]->fts_link = NULL;

    return newhead;
}

static FTSENT* fts_alloc(FTS* sp, const char* name, size_t namelen) {
    size_t len = sizeof(FTSENT) + namelen + 1;

    if (!ISSET(FTS_NOSTAT)) {
        len += sizeof(struct stat) + ALIGNBYTES;
    }

    FTSENT* p = calloc(1, len);
    if (!p)
        return NULL;

    p->fts_path = sp->fts_path;
    p->fts_namelen = namelen;
    p->fts_instr = FTS_NOINSTR;

    if (!ISSET(FTS_NOSTAT)) {
        uintptr_t base = (uintptr_t)&p->fts_name[namelen + 1];
        base = ALIGN(base);
        p->fts_statp = (struct stat*)base;
    }

    memcpy(p->fts_name, name, namelen);
    p->fts_name[namelen] = '\0';
    return p;
}

static void fts_lfree(FTSENT* head) {
    while (head) {
        FTSENT* p = head;
        head = head->fts_link;
        free(p);
    }
}

static int fts_palloc(FTS* sp, size_t more) {
    const size_t pad = 256;
    size_t need = sp->fts_pathlen + more + pad;
    if (need < sp->fts_pathlen) {
        free(sp->fts_path);
        sp->fts_path = NULL;
        errno = ENAMETOOLONG;
        return 1;
    }

    void* r = safe_recallocarray(sp->fts_path, sp->fts_pathlen, need, 1);
    if (!r) {
        free(sp->fts_path);
        sp->fts_path = NULL;
        return 1;
    }
    sp->fts_path = r;
    sp->fts_pathlen = need;
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

    for (FTSENT* p = sp->fts_child; p; p = p->fts_link) {
        ADJUST(p);
    }

    for (FTSENT* p = head; p->fts_level >= FTS_ROOTLEVEL;) {
        ADJUST(p);
        if (p->fts_link)
            p = p->fts_link;
        else
            p = p->fts_parent;
    }
}

static size_t fts_maxarglen(char* const* argv) {
    size_t max = 0;
    for (; *argv; argv++) {
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
        newfd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (newfd == -1)
            return -1;
    }

    struct stat sb;
    if (fstat(newfd, &sb) == -1) {
        int oerrno = errno;
        if (fd == -1)
            close(newfd);
        errno = oerrno;
        return -1;
    }
    if (p->fts_dev != sb.st_dev || p->fts_ino != sb.st_ino) {
        errno = ENOENT;
        if (fd == -1)
            close(newfd);
        return -1;
    }

    int ret = fchdir(newfd);
    int oerrno = errno;
    if (fd == -1)
        close(newfd);
    errno = oerrno;
    return ret;
}

static void fts_load(FTS* sp, FTSENT* p) {
    size_t len;
    char* cp;

    len = p->fts_pathlen = p->fts_namelen;
    memmove(sp->fts_path, p->fts_name, len + 1);

    if ((cp = strrchr(p->fts_name, '/')) && (cp != p->fts_name || cp[1])) {
        len = strlen(++cp);
        memmove(p->fts_name, cp, len + 1);
        p->fts_namelen = len;
    }

    p->fts_accpath = p->fts_path = sp->fts_path;
    sp->fts_dev = p->fts_dev;
}

#ifdef __cplusplus
}
#endif
