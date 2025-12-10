#include "fts_test_common.h"
#include "musl-bsd/fts_ops.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct fd_stats {
    int opens;
    int closes;
    int closedirs;
    int fchdirs;
    int open_dot;
    int current;
    int max_current;
};

static struct fd_stats stats;

static void fd_stats_reset(void) {
    memset(&stats, 0, sizeof(stats));
}

static void fd_note_open(const char* path, int fd) {
    if (fd < 0)
        return;
    stats.opens++;
    stats.current++;
    if (stats.current > stats.max_current)
        stats.max_current = stats.current;
    if (path && strcmp(path, ".") == 0)
        stats.open_dot++;
}

static int tracking_open(const char* path, int flags) {
    int fd = open(path, flags);
    fd_note_open(path, fd);
    return fd;
}

static int tracking_close(int fd) {
    int rc = close(fd);
    if (rc == 0) {
        stats.closes++;
        stats.current--;
    }
    return rc;
}

static int tracking_fstat(int fd, struct stat* st) {
    return fstat(fd, st);
}

static int tracking_fstatat(int dirfd, const char* path, struct stat* st, int flags) {
    return fstatat(dirfd, path, st, flags);
}

static int tracking_fchdir(int fd) {
    int rc = fchdir(fd);
    if (rc == 0)
        stats.fchdirs++;
    return rc;
}

static DIR* tracking_fdopendir(int fd) {
    DIR* d = fdopendir(fd);
    if (d)
        stats.max_current = stats.current > stats.max_current ? stats.current : stats.max_current;
    return d;
}

static struct dirent* tracking_readdir(DIR* dirp) {
    return readdir(dirp);
}

static int tracking_closedir(DIR* dirp) {
    int rc = closedir(dirp);
    if (rc == 0) {
        stats.closedirs++;
        stats.current--;
    }
    return rc;
}

static const struct fts_ops tracking_ops = {
    .open_fn = tracking_open,
    .close_fn = tracking_close,
    .fstat_fn = tracking_fstat,
    .fstatat_fn = tracking_fstatat,
    .fchdir_fn = tracking_fchdir,
    .fdopendir_fn = tracking_fdopendir,
    .readdir_fn = tracking_readdir,
    .closedir_fn = tracking_closedir
};

extern const struct fts_ops* __fts_ops_override;

static void assert_fd_discipline(const char* label, bool expect_chdir) {
    fts_check(stats.current == 0, "%s leaves no fds open (balance=%d)", label, stats.current);
    fts_check(stats.opens == stats.closes + stats.closedirs,
              "%s balances open (%d) with close (%d) and closedir (%d)", label, stats.opens, stats.closes, stats.closedirs);

    if (expect_chdir) {
        fts_check(stats.fchdirs > 0, "%s performed at least one fchdir", label);
        fts_check(stats.open_dot > 0, "%s opened cwd descriptor in chdir mode", label);
    }
    else {
        fts_check(stats.fchdirs == 0, "%s avoided fchdir under FTS_NOCHDIR", label);
        fts_check(stats.open_dot == 0, "%s did not open \".\" under FTS_NOCHDIR", label);
    }
}

static void run_probe(const char* label, char* const* roots, int opts, int stop_after, bool expect_chdir) {
    fd_stats_reset();
    __fts_ops_override = &tracking_ops;

    FTS* f = fts_open(roots, opts, NULL);
    fts_check(f != NULL, "%s fts_open", label);
    int visited = 0;
    if (f) {
        FTSENT* e;
        while ((e = fts_read(f)) != NULL) {
            visited++;
            if (stop_after > 0 && visited >= stop_after)
                break;
        }
        fts_check(visited > 0, "%s visited entries", label);
        fts_check(fts_close(f) == 0, "%s fts_close", label);
    }

    __fts_ops_override = NULL;
    assert_fd_discipline(label, expect_chdir);
}

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return fts_exit_code();

    char** roots = fts_make_roots(tree.abs_root, NULL);
    if (!roots) {
        fts_test_tree_cleanup(&tree);
        return fts_exit_code();
    }

    run_probe("NOCHDIR full walk", roots, FTS_PHYSICAL | FTS_NOCHDIR, 0, false);
    run_probe("NOCHDIR early stop", roots, FTS_PHYSICAL | FTS_NOCHDIR, 2, false);
    run_probe("chdir full walk", roots, FTS_PHYSICAL, 0, true);
    run_probe("chdir early stop", roots, FTS_PHYSICAL, 2, true);

    free(roots);
    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
