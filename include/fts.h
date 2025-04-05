/*
 * fts.h
 */

#ifndef FTS_H
#define FTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h> /* for dev_t, ino_t, off_t */
#include <sys/stat.h>  /* for struct stat */
#include <stddef.h>    /* for size_t */

/*----------------------------------------------------------------------*
 * fts->fts_options flags (bitmask)
 *----------------------------------------------------------------------*/
#define FTS_COMFOLLOW 0x0001 /* Follow command line symlinks on first lookup */
#define FTS_LOGICAL 0x0002   /* Follow symlinks logically */
#define FTS_NOCHDIR 0x0004   /* Do not change directories as we traverse */
#define FTS_NOSTAT 0x0008    /* Do not stat each file in the hierarchy */
#define FTS_PHYSICAL 0x0010  /* Physical walk, ignoring symlinks except on cmd line */
#define FTS_SEEDOT 0x0020    /* Return . and .. even if they appear in a directory */
#define FTS_XDEV 0x0040      /* Do not cross devices when descending directories */

/* Used internally; not user-settable. */
#define FTS_STOP 0x2000

/*----------------------------------------------------------------------*
 * ftsent->fts_info values
 *----------------------------------------------------------------------*/
#define FTS_D 1       /* A directory being visited pre-order */
#define FTS_DC 2      /* A directory that causes a cycle (loop) */
#define FTS_DEFAULT 3 /* None of the other FTS_* constants apply */
#define FTS_DNR 4     /* A directory which cannot be read */
#define FTS_ERR 5     /* An error condition (errno set) */
#define FTS_F 6       /* A regular file */
#define FTS_INIT 7    /* Initialization state (internal) */
#define FTS_NS 8      /* stat(2) failed */
#define FTS_NSOK 9    /* No stat requested (FTS_NOSTAT), so no info */
#define FTS_SL 10     /* A symbolic link */
#define FTS_SLNONE 11 /* A symlink with no target (or stat failed) */
#define FTS_DP 12     /* A directory being visited post-order */
#define FTS_DOT 13    /* The name is "." or ".." */

/*----------------------------------------------------------------------*
 * fts_set() instructions
 *----------------------------------------------------------------------*/
#define FTS_AGAIN 1   /* Re-stat the node in a subsequent fts_read() */
#define FTS_FOLLOW 2  /* If it is a symlink, follow it once */
#define FTS_NOINSTR 3 /* Do nothing special */
#define FTS_SKIP 4    /* Skip any remaining traversal of this directory */

/*----------------------------------------------------------------------*
 * fts_children() instructions
 *----------------------------------------------------------------------*/
#define FTS_NAMEONLY 1 /* Only build up child names, don’t stat them */

/*----------------------------------------------------------------------*
 * Forward declarations
 *----------------------------------------------------------------------*/
struct _fts;    /* The opaque FTS structure */
struct _ftsent; /* The FTSENT structure */

/*----------------------------------------------------------------------*
 * The FTSENT structure
 *
 * For portable code, you typically only access these fields:
 *  fts_info, fts_path, fts_accpath, fts_statp, fts_errno, fts_level,
 *  fts_name, fts_namelen, fts_parent, fts_link, fts_cycle.
 *
 * The rest are used internally by the library.
 *----------------------------------------------------------------------*/
typedef struct _ftsent {
    struct _ftsent* fts_cycle;  /* Cycle start, if any */
    struct _ftsent* fts_parent; /* Parent directory entry */
    struct _ftsent* fts_link;   /* Next file in directory */
    struct stat* fts_statp;     /* Stat(2) data (may be NULL if FTS_NOSTAT) */
    char* fts_accpath;          /* Access path (relative or absolute) */
    char* fts_path;             /* Full path buffer, mutable */
    off_t fts_nlink;            /* Link count (for directories) */
    dev_t fts_dev;              /* Device number */
    ino_t fts_ino;              /* Inode number */
    int fts_symfd;              /* FD if following a symlink directory */
    short fts_info;             /* Status flags for this entry */
    short fts_flags;            /* Private flags, internal use */
    short fts_instr;            /* fts_set() instruction */
    short fts_level;            /* Depth in the traversal tree */
    int fts_errno;              /* Errno for this node (if fts_info == FTS_ERR) */
    size_t fts_pathlen;         /* Current length of fts_path */
    size_t fts_namelen;         /* Length of fts_name */
    char fts_name[];            /* Filename (flexible array member) */
} FTSENT;

/*----------------------------------------------------------------------*
 * The main FTS structure
 *
 * Typically treated as opaque by user code.
 *----------------------------------------------------------------------*/
typedef struct _fts {
    FTSENT* fts_cur;                                   /* Current node being processed */
    FTSENT* fts_child;                                 /* Linked list of children */
    FTSENT** fts_array;                                /* Array for sorting children */
    int fts_nitems;                                    /* Number of elements in fts_array */
    char* fts_path;                                    /* Reusable path buffer */
    size_t fts_pathlen;                                /* Current allocated length of fts_path */
    int fts_rfd;                                       /* Saved root FD for fchdir() */
    int fts_options;                                   /* Options set in fts_open() */
    dev_t fts_dev;                                     /* Device of the initial dir, for -xdev */
    int (*fts_compar)(const FTSENT**, const FTSENT**); /* Sort function */
} FTS;

/*----------------------------------------------------------------------*
 * Prototypes for the public fts(3)-like functions
 *----------------------------------------------------------------------*/

/**
 * Open a file hierarchy for traversal. The argv array lists paths
 * to traverse. `options` is a bitmask of FTS_* flags. `compar` is an
 * optional comparison function for sorting each directory’s contents.
 * Returns an FTS pointer on success, or NULL on error.
 */
FTS* fts_open(char* const* argv, int options, int (*compar)(const FTSENT**, const FTSENT**));

/**
 * Read the next entry from the hierarchy. Returns an FTSENT pointer,
 * or NULL on end or error.
 */
FTSENT* fts_read(FTS*);

/**
 * Obtain a linked list of the children of the current directory being
 * visited by fts_read(). May return NULL on error or if none. The `instr`
 * argument can be FTS_NAMEONLY to skip stat(2).
 */
FTSENT* fts_children(FTS*, int instr);

/**
 * Control the traversal of a given node. The instruction can be one of:
 *   FTS_AGAIN, FTS_FOLLOW, FTS_NOINSTR, FTS_SKIP.
 * Returns 0 on success, non-zero on error (e.g. invalid instruction).
 */
int fts_set(FTS*, FTSENT*, int instr);

/**
 * Close an FTS traversal. Frees internal data and returns 0 on success,
 * or -1 if unable to restore directory (errno set).
 */
int fts_close(FTS*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FTS_H */
