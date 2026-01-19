/* Internal/test-only syscall shim for fts; not part of the public API surface. */

#ifndef MUSL_BSD_FTS_OPS_H
#define MUSL_BSD_FTS_OPS_H

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct fts_ops {
    int (*open_fn)(const char*, int);
    int (*close_fn)(int);
    int (*fstat_fn)(int, struct stat*);
    int (*fstatat_fn)(int, const char*, struct stat*, int);
    int (*fchdir_fn)(int);
    DIR* (*fdopendir_fn)(int);
    struct dirent* (*readdir_fn)(DIR*);
    int (*closedir_fn)(DIR*);
};

/* Optional override used by tests for fault injection; leave NULL for defaults.
   This hook is internal and not thread-safe. */
extern const struct fts_ops* __fts_ops_override;

#ifdef __cplusplus
}
#endif

#endif /* MUSL_BSD_FTS_OPS_H */
