/* SPDX‑License‑Identifier: BSD */

#ifndef FTS_H
#define FTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 * Caller‑visible option flags (same numeric values as BSD).
 * May be OR’ed together when calling fts_open().
 */
#define FTS_COMFOLLOW 0x0001
#define FTS_LOGICAL 0x0002
#define FTS_NOCHDIR 0x0004
#define FTS_NOSTAT 0x0008
#define FTS_PHYSICAL 0x0010
#define FTS_SEEDOT 0x0020
#define FTS_XDEV 0x0040
#define FTS_STOP 0x2000 /* (internal – do not pass to open) */

/* fts_info values returned in FTSENT.fts_info */
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

/* fts_set() instructions */
#define FTS_AGAIN 1
#define FTS_FOLLOW 2
#define FTS_NOINSTR 3
#define FTS_SKIP 4

/* fts_children() selector */
#define FTS_NAMEONLY 1

/* Public traversal structures */
typedef struct _ftsent {
    struct _ftsent* fts_cycle;
    struct _ftsent* fts_parent;
    struct _ftsent* fts_link;
    struct stat* fts_statp;
    char* fts_accpath; /* path as presented to fts(3) user       */
    char* fts_path;    /* mutable traversal buffer (read‑only)   */
    off_t fts_nlink;
    dev_t fts_dev;
    ino_t fts_ino;
    int fts_symfd; /* internal – fd to dir when following SL */
    short fts_info;
    short fts_flags; /* private implementation flags           */
    short fts_instr; /* per‑entry instruction                  */
    short fts_level; /* depth – root == 0                      */
    int fts_errno;   /* errno from failed stat/op              */
    size_t fts_pathlen;
    size_t fts_namelen;
    char fts_name[]; /* flexible array member                  */
} FTSENT;

typedef struct _fts {
    FTSENT* fts_cur;    /* current node returned by fts_read()    */
    FTSENT* fts_child;  /* internal child list cache              */
    FTSENT** fts_array; /* scratch array for sorting              */
    int fts_nitems;     /* size of fts_array                      */
    char* fts_path;     /* shared path buffer                     */
    size_t fts_pathlen;
    int fts_rfd; /* original cwd fd (when !NOCHDIR)        */
    int fts_options;
    dev_t fts_dev; /* device of root dir when FTS_XDEV       */
    int (*fts_compar)(const FTSENT**, const FTSENT**);
} FTS;

/* API – **NOT** async‑signal‑safe, **NOT** thread‑safe (uses chdir). */
FTS* fts_open(char* const* argv, int options, int (*compar)(const FTSENT**, const FTSENT**));
FTSENT* fts_read(FTS*);
FTSENT* fts_children(FTS*, int instr);
int fts_set(FTS*, FTSENT*, int instr);
int fts_close(FTS*);

#ifdef __cplusplus
}
#endif
#endif /* FTS_H */
