#include "fts_test_common.h"
#include "musl-bsd/fts_ops.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static char* fail_path;
static int fail_errno_val;
static int inject_hits;

static int injected_open(const char* path, int flags) {
    if (fail_path && strcmp(path, fail_path) == 0) {
        inject_hits++;
        errno = fail_errno_val;
        return -1;
    }
    return open(path, flags);
}

static const struct fts_ops injected_ops = {
    .open_fn = injected_open,
    .close_fn = close,
    .fstat_fn = fstat,
    .fstatat_fn = fstatat,
    .fchdir_fn = fchdir,
    .fdopendir_fn = fdopendir,
    .readdir_fn = readdir,
    .closedir_fn = closedir
};

extern const struct fts_ops* __fts_ops_override;

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    fail_errno_val = EIO;
    fail_path = fts_join2(tree.abs_root, "b");
    if (!fail_path) {
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    __fts_ops_override = &injected_ops;

    char* roots[] = {tree.abs_root, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    fts_check(f != NULL, "fts_open with injected ops shim");

    bool saw_b = false;
    bool saw_b_dnr = false;
    int saw_errno = 0;
    if (f) {
        FTSENT* e;
        while ((e = fts_read(f)) != NULL) {
            if (strcmp(e->fts_name, "b") == 0) {
                saw_b = true;
                if (e->fts_info == FTS_DNR) {
                    saw_b_dnr = true;
                    saw_errno = e->fts_errno;
                }
            }
        }
        fts_check(fts_close(f) == 0, "fts_close succeeds after injected failure");
    }

    fts_check(saw_b, "walk visited the failing node");
    fts_check(saw_b_dnr, "open failure surfaces as FTS_DNR");
    fts_check(!saw_b_dnr || saw_errno == fail_errno_val, "errno propagated from injected open failure");
    fts_check(inject_hits == 1, "open shim fired exactly once");

    fts_test_tree_cleanup(&tree);
    free(fail_path);
    return fts_exit_code();
}
