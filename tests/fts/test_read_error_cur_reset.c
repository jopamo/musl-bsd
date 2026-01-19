#include "fts_test_common.h"
#include "musl-bsd/fts_ops.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fail_once = 1;

static int injected_fchdir(int fd) {
    if (fail_once) {
        fail_once = 0;
        errno = EIO;
        return -1;
    }
    return fchdir(fd);
}

static int injected_open(const char* path, int flags) {
    return open(path, flags);
}

static const struct fts_ops injected_ops = {.open_fn = injected_open,
                                            .close_fn = close,
                                            .fstat_fn = fstat,
                                            .fstatat_fn = fstatat,
                                            .fchdir_fn = injected_fchdir,
                                            .fdopendir_fn = fdopendir,
                                            .readdir_fn = readdir,
                                            .closedir_fn = closedir};

extern const struct fts_ops* __fts_ops_override;

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    __fts_ops_override = &injected_ops;

    char* roots[] = {tree.abs_root, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL, NULL);
    fts_check(f != NULL, "fts_open for fchdir failure");

    if (f) {
        errno = 0;
        FTSENT* e = fts_read(f);
        fts_check(e == NULL, "fts_read returns NULL on injected fchdir failure");
        fts_check(errno == EIO, "fts_read propagated injected errno");
        fts_check(fts_close(f) == 0, "fts_close succeeds after early fts_read failure");
    }

    __fts_ops_override = NULL;

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
