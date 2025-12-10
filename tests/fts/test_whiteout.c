#include "fts_test_common.h"
#include "musl-bsd/fts_ops.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef DT_WHT
int main(void) {
    fprintf(stderr, "DT_WHT not available; skipping whiteout coverage\n");
    return 0;
}
#else

static const char* whiteout_name;

static struct dirent* mark_whiteout_readdir(DIR* dirp) {
    struct dirent* d = readdir(dirp);
    if (d && whiteout_name && strcmp(d->d_name, whiteout_name) == 0)
        d->d_type = DT_WHT;
    return d;
}

static int whiteout_open(const char* path, int flags) {
    return open(path, flags);
}

static const struct fts_ops whiteout_ops = {
    .open_fn = whiteout_open,
    .close_fn = close,
    .fstat_fn = fstat,
    .fstatat_fn = fstatat,
    .fchdir_fn = fchdir,
    .fdopendir_fn = fdopendir,
    .readdir_fn = mark_whiteout_readdir,
    .closedir_fn = closedir
};

extern const struct fts_ops* __fts_ops_override;

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    whiteout_name = "whiteout_marker";
    char* marker_path = fts_join2(tree.abs_root, whiteout_name);
    if (!marker_path) {
        fts_test_tree_cleanup(&tree);
        return 1;
    }
    if (fts_write_file(marker_path, "marker\n") == -1) {
        free(marker_path);
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    __fts_ops_override = &whiteout_ops;

    char* roots[] = {tree.abs_root, NULL};
    bool saw_whiteout = false;

    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR | FTS_WHITEOUT, NULL);
    fts_check(f != NULL, "fts_open with FTS_WHITEOUT");
    if (f) {
        FTSENT* e;
        while ((e = fts_read(f)) != NULL) {
            if (strcmp(e->fts_name, whiteout_name) == 0) {
                saw_whiteout = true;
                fts_check(e->fts_info == FTS_W, "whiteout entry reported as FTS_W");
                fts_check((e->fts_flags & FTS_ISW) != 0, "whiteout flag set on entry");
#ifdef S_IFWHT
                fts_check(e->fts_statp->st_mode == S_IFWHT, "whiteout stat mode flagged");
#else
                fts_check(e->fts_statp->st_mode == 0, "whiteout stat cleared without S_IFWHT");
#endif
                fts_check(e->fts_errno == 0, "whiteout errno clear");
                break;
            }
        }
        fts_check(fts_close(f) == 0, "fts_close with FTS_WHITEOUT");
    }
    fts_check(saw_whiteout, "whiteout node visited during walk");

    bool saw_regular = false;
    f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    fts_check(f != NULL, "fts_open without FTS_WHITEOUT");
    if (f) {
        FTSENT* e;
        while ((e = fts_read(f)) != NULL) {
            if (strcmp(e->fts_name, whiteout_name) == 0) {
                saw_regular = true;
                fts_check(e->fts_info == FTS_F, "entry without FTS_WHITEOUT treated as regular file");
                fts_check((e->fts_flags & FTS_ISW) == 0, "FTS_ISW not set when option disabled");
                break;
            }
        }
        fts_check(fts_close(f) == 0, "fts_close without FTS_WHITEOUT");
    }
    fts_check(saw_regular, "regular walk saw marker");

    __fts_ops_override = NULL;

    free(marker_path);
    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
#endif
