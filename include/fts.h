/* SPDX-License-Identifier: BSD */

#ifndef FTS_H
#define FTS_H

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _ftsent;

#ifndef __fts_stat_t
#define __fts_stat_t struct stat
#endif
#ifndef __fts_nlink_t
#define __fts_nlink_t nlink_t
#endif
#ifndef __fts_ino_t
#define __fts_ino_t ino_t
#endif
#ifndef __fts_length_t
#define __fts_length_t unsigned int
#endif
#ifndef __fts_number_t
#define __fts_number_t int64_t
#endif
#ifndef __fts_dev_t
#define __fts_dev_t dev_t
#endif
#ifndef __fts_level_t
#define __fts_level_t int
#endif

typedef struct _fts {
    struct _ftsent* fts_cur;             /* current node */
    struct _ftsent* fts_child;           /* linked list of children */
    struct _ftsent** fts_array;          /* sort array */
    __fts_dev_t fts_dev;                 /* starting device # */
    char* fts_path;                      /* path for this descent */
    int fts_rfd;                         /* fd for root */
    __fts_length_t fts_pathlen;          /* sizeof(path) */
    __fts_length_t fts_nitems;           /* elements in the sort array */
    int (*fts_compar)(const struct _ftsent**, const struct _ftsent**);

#define FTS_COMFOLLOW 0x0001
#define FTS_LOGICAL 0x0002
#define FTS_NOCHDIR 0x0004
#define FTS_NOSTAT 0x0008
#define FTS_PHYSICAL 0x0010
#define FTS_SEEDOT 0x0020
#define FTS_XDEV 0x0040
#define FTS_WHITEOUT 0x0080
#define FTS_OPTIONMASK 0x00ff

#define FTS_NAMEONLY 0x0100
#define FTS_STOP 0x0200
    int fts_options;
} FTS;

typedef struct _ftsent {
    struct _ftsent* fts_cycle; /* cycle node */
    struct _ftsent* fts_parent; /* parent directory */
    struct _ftsent* fts_link; /* next file in directory */
    __fts_number_t fts_number; /* local numeric value */
    void* fts_pointer; /* local address value */
    char* fts_accpath; /* access path */
    char* fts_path; /* root path */
    int fts_errno; /* errno for this node */
    int fts_symfd; /* fd for symlink */
    __fts_length_t fts_pathlen; /* strlen(fts_path) */
    __fts_length_t fts_namelen; /* strlen(fts_name) */

    __fts_ino_t fts_ino; /* inode */
    __fts_dev_t fts_dev; /* device */
    __fts_nlink_t fts_nlink; /* link count */

#define FTS_ROOTPARENTLEVEL -1
#define FTS_ROOTLEVEL 0
    __fts_level_t fts_level; /* depth (-1 to N) */

#define FTS_D 1
#define FTS_DC 2
#define FTS_DEFAULT 3
#define FTS_DNR 4
#define FTS_DOT 5
#define FTS_DP 6
#define FTS_ERR 7
#define FTS_F 8
#define FTS_INIT 9
#define FTS_NS 10
#define FTS_NSOK 11
#define FTS_SL 12
#define FTS_SLNONE 13
#define FTS_W 14
    unsigned short fts_info;

#define FTS_DONTCHDIR 0x01
#define FTS_SYMFOLLOW 0x02
#define FTS_ISW 0x04
    unsigned short fts_flags;

#define FTS_AGAIN 1
#define FTS_FOLLOW 2
#define FTS_NOINSTR 3
#define FTS_SKIP 4
    unsigned short fts_instr;

    __fts_stat_t* fts_statp; /* stat(2) information */
    char fts_name[1]; /* file name */
} FTSENT;

FTS* fts_open(char* const* argv, int options, int (*compar)(const FTSENT**, const FTSENT**));
FTSENT* fts_read(FTS*);
FTSENT* fts_children(FTS*, int);
int fts_set(FTS*, FTSENT*, int);
int fts_close(FTS*);

#ifdef __cplusplus
}
#endif

#endif /* FTS_H */
