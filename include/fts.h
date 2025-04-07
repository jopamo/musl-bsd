#ifndef FTS_H
#define FTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>

#define FTS_COMFOLLOW 0x0001
#define FTS_LOGICAL 0x0002
#define FTS_NOCHDIR 0x0004
#define FTS_NOSTAT 0x0008
#define FTS_PHYSICAL 0x0010
#define FTS_SEEDOT 0x0020
#define FTS_XDEV 0x0040

#define FTS_STOP 0x2000

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

struct _fts;
struct _ftsent;

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

FTSENT* fts_children(FTS*, int instr);

int fts_set(FTS*, FTSENT*, int instr);

int fts_close(FTS*);

#ifdef __cplusplus
}
#endif

#endif
