#include "fts_test_common.h"
#include "musl-bsd/fts_ops.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void* __real_malloc(size_t size);

static char* fail_open_path;
static int fail_open_errno;
static int fail_fstat_once;
static int fail_fchdir_once;
static int fail_readdir_once;
static int fail_malloc_once;
static int open_fail_hits;

void* __wrap_malloc(size_t size) {
    if (fail_malloc_once) {
        fail_malloc_once = 0;
        errno = ENOMEM;
        return NULL;
    }
    return __real_malloc(size);
}

static void reset_injection(void) {
    fail_open_path = NULL;
    fail_open_errno = EIO;
    fail_fstat_once = 0;
    fail_fchdir_once = 0;
    fail_readdir_once = 0;
    fail_malloc_once = 0;
    open_fail_hits = 0;
}

static int injected_open(const char* path, int flags) {
    if (fail_open_path && strcmp(path, fail_open_path) == 0) {
        open_fail_hits++;
        errno = fail_open_errno;
        return -1;
    }
    return open(path, flags);
}

static int injected_fstat(int fd, struct stat* st) {
    if (fail_fstat_once) {
        fail_fstat_once = 0;
        errno = EIO;
        return -1;
    }
    return fstat(fd, st);
}

static int injected_fchdir(int fd) {
    if (fail_fchdir_once) {
        fail_fchdir_once = 0;
        errno = EIO;
        return -1;
    }
    return fchdir(fd);
}

static struct dirent* injected_readdir(DIR* dirp) {
    if (fail_readdir_once) {
        fail_readdir_once = 0;
        errno = EIO;
        return NULL;
    }
    return readdir(dirp);
}

static const struct fts_ops injected_ops = {.open_fn = injected_open,
                                            .close_fn = close,
                                            .fstat_fn = injected_fstat,
                                            .fstatat_fn = fstatat,
                                            .fchdir_fn = injected_fchdir,
                                            .fdopendir_fn = fdopendir,
                                            .readdir_fn = injected_readdir,
                                            .closedir_fn = closedir};

extern const struct fts_ops* __fts_ops_override;

static FTS* open_walk(const struct fts_test_tree* tree, int opts, const char* label) {
    char* roots[] = {tree->abs_root, NULL};
    FTS* f = fts_open(roots, opts, NULL);
    fts_check(f != NULL, "%s: fts_open succeeds", label);
    return f;
}

static void test_open_failure(const struct fts_test_tree* tree) {
    reset_injection();

    fail_open_path = fts_join2(tree->abs_root, "b");
    fts_check(fail_open_path != NULL, "open failure: allocated injected path");
    if (!fail_open_path)
        return;

    FTS* f = open_walk(tree, FTS_PHYSICAL | FTS_NOCHDIR, "open failure");
    bool saw_b = false;
    bool saw_dnr = false;
    int saw_errno = 0;

    if (f) {
        FTSENT* e;
        while ((e = fts_read(f)) != NULL) {
            if (strcmp(e->fts_name, "b") != 0)
                continue;
            saw_b = true;
            if (e->fts_info == FTS_DNR) {
                saw_dnr = true;
                saw_errno = e->fts_errno;
            }
        }
        fts_check(fts_close(f) == 0, "open failure: fts_close succeeds");
    }

    fts_check(saw_b, "open failure: visited injected directory");
    fts_check(saw_dnr, "open failure: surfaces as FTS_DNR");
    fts_check(!saw_dnr || saw_errno == EIO, "open failure: propagated errno");
    fts_check(open_fail_hits == 1, "open failure: injected open fired once");

    free(fail_open_path);
    fail_open_path = NULL;
}

static void test_fstat_failure(const struct fts_test_tree* tree) {
    reset_injection();

    FTS* f = open_walk(tree, FTS_PHYSICAL | FTS_NOCHDIR, "fstat failure");
    if (!f)
        return;

    FTSENT* root = fts_read(f);
    fts_check(root && root->fts_info == FTS_D, "fstat failure: first read returns root dir");

    fail_fstat_once = 1;
    errno = 0;
    FTSENT* e = fts_read(f);
    fts_check(e == root, "fstat failure: second read returns current node");
    fts_check(e && e->fts_info == FTS_ERR, "fstat failure: root promoted to FTS_ERR");
    fts_check(e && e->fts_errno == EIO, "fstat failure: node stores injected errno");
    fts_check(errno == EIO, "fstat failure: errno propagated");

    fts_check(fts_close(f) == 0, "fstat failure: fts_close succeeds");
}

static void test_chdir_failure(const struct fts_test_tree* tree) {
    reset_injection();

    FTS* f = open_walk(tree, FTS_PHYSICAL, "chdir failure");
    if (!f)
        return;

    FTSENT* root = fts_read(f);
    fts_check(root && root->fts_info == FTS_D, "chdir failure: first read returns root dir");

    fail_fchdir_once = 1;
    bool saw_ns = false;
    FTSENT* e;
    while ((e = fts_read(f)) != NULL) {
        if (e->fts_level == 1 && e->fts_info == FTS_NS && e->fts_errno == EIO) {
            saw_ns = true;
            break;
        }
    }
    fts_check(saw_ns, "chdir failure: children surface as FTS_NS with injected errno");
    fts_check(fts_close(f) == 0, "chdir failure: fts_close succeeds");
}

static void test_readdir_failure(const struct fts_test_tree* tree) {
    reset_injection();

    FTS* f = open_walk(tree, FTS_PHYSICAL | FTS_NOCHDIR, "readdir failure");
    if (!f)
        return;

    FTSENT* root = fts_read(f);
    fts_check(root && root->fts_info == FTS_D, "readdir failure: first read returns root dir");

    fail_readdir_once = 1;
    errno = 0;
    FTSENT* e = fts_read(f);
    fts_check(e == NULL, "readdir failure: traversal stops immediately");
    fts_check(errno == EIO, "readdir failure: errno propagated");
    fts_check(root && root->fts_info == FTS_ERR, "readdir failure: root promoted to FTS_ERR");

    fts_check(fts_close(f) == 0, "readdir failure: fts_close succeeds");
}

static void test_enomem_cycle_insert(const struct fts_test_tree* tree) {
    reset_injection();

    FTS* f = open_walk(tree, FTS_PHYSICAL | FTS_NOCHDIR, "ENOMEM failure");
    if (!f)
        return;

    FTSENT* root = fts_read(f);
    fts_check(root && root->fts_info == FTS_D, "ENOMEM failure: first read returns root dir");

    fail_malloc_once = 1;
    errno = 0;
    FTSENT* e = fts_read(f);
    fts_check(e == root, "ENOMEM failure: second read returns current node");
    fts_check(e && e->fts_info == FTS_ERR, "ENOMEM failure: cycle insertion surfaces as FTS_ERR");
    fts_check(e && e->fts_errno == ENOMEM, "ENOMEM failure: node stores ENOMEM");
    fts_check(errno == ENOMEM, "ENOMEM failure: errno propagated");

    e = fts_read(f);
    fts_check(e == NULL, "ENOMEM failure: traversal stopped after fatal allocation failure");
    fts_check(fts_close(f) == 0, "ENOMEM failure: fts_close succeeds");
}

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    __fts_ops_override = &injected_ops;

    test_open_failure(&tree);
    test_fstat_failure(&tree);
    test_chdir_failure(&tree);
    test_readdir_failure(&tree);
    test_enomem_cycle_insert(&tree);

    __fts_ops_override = NULL;
    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
