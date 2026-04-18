#include "fts_test_common.h"
#include "musl-bsd/fts_ops.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static const char* fail_path_once;
static int fail_path_errno;
static int fail_path_hits;
static int fail_dotdot_once;
static int fail_dotdot_hits;

static void reset_injection(void) {
    fail_path_once = NULL;
    fail_path_errno = EIO;
    fail_path_hits = 0;
    fail_dotdot_once = 0;
    fail_dotdot_hits = 0;
}

static int injected_open(const char* path, int flags) {
    if (fail_path_once && strcmp(path, fail_path_once) == 0) {
        fail_path_once = NULL;
        fail_path_hits++;
        errno = fail_path_errno;
        return -1;
    }

    if (fail_dotdot_once && strcmp(path, "..") == 0) {
        fail_dotdot_once = 0;
        fail_dotdot_hits++;
        errno = EIO;
        return -1;
    }

    return open(path, flags);
}

static const struct fts_ops injected_ops = {.open_fn = injected_open,
                                            .close_fn = close,
                                            .fstat_fn = fstat,
                                            .fstatat_fn = fstatat,
                                            .fchdir_fn = fchdir,
                                            .fdopendir_fn = fdopendir,
                                            .readdir_fn = readdir,
                                            .closedir_fn = closedir};

extern const struct fts_ops* __fts_ops_override;

static FTS* open_walk(const struct fts_test_tree* tree, int opts, const char* label) {
    char* roots[] = {tree->abs_root, NULL};
    FTS* f = fts_open(roots, opts, NULL);
    fts_check(f != NULL, "%s: fts_open succeeds", label);
    return f;
}

static void test_prebuilt_children_changedir_fallback(const struct fts_test_tree* tree) {
    reset_injection();

    FTS* f = open_walk(tree, FTS_PHYSICAL, "fallback");
    if (!f)
        return;

    FTSENT* root = fts_read(f);
    fts_check(root && root->fts_info == FTS_D, "fallback: first read returns root directory");
    if (!root || root->fts_info != FTS_D) {
        fts_close(f);
        return;
    }

    FTSENT* kids = fts_children(f, 0);
    fts_check(kids != NULL, "fallback: fts_children preloads root children");
    if (!kids) {
        fts_close(f);
        return;
    }

    fail_path_once = tree->abs_root;
    fail_path_errno = EIO;
    FTSENT* first = fts_read(f);

    fts_check(first != NULL, "fallback: read still returns a child after changedir failure");
    fts_check(fail_path_hits == 1, "fallback: reopen injection fired once");
    fts_check((root->fts_flags & FTS_DONTCHDIR) != 0, "fallback: parent marked FTS_DONTCHDIR");
    fts_check(root->fts_errno == EIO, "fallback: parent records changedir errno");
    fts_check(first && first->fts_parent == root, "fallback: returned node is a root child");
    fts_check(first && first->fts_accpath == root->fts_accpath, "fallback: child accpath falls back to parent accpath");

    fts_check(fts_close(f) == 0, "fallback: fts_close succeeds");
}

static void test_parent_changedir_error_stops_walk(const struct fts_test_tree* tree) {
    reset_injection();
    fail_dotdot_once = 1;

    FTS* f = open_walk(tree, FTS_PHYSICAL, "error");
    if (!f)
        return;

    bool saw_level1_dir = false;
    FTSENT* e;
    errno = 0;
    while ((e = fts_read(f)) != NULL) {
        if (e->fts_level == 1 && e->fts_info == FTS_D)
            saw_level1_dir = true;
    }
    int saved_errno = errno;

    fts_check(saw_level1_dir, "error: traversal reached a nested directory before failure");
    fts_check(saved_errno == EIO, "error: propagated errno from failed \"..\" reopen");
    fts_check(fail_dotdot_hits == 1, "error: \"..\" reopen injection fired once");
    fts_check((f->fts_options & FTS_STOP) != 0, "error: traversal enters FTS_STOP state");
    fts_check(f->fts_cur != NULL && f->fts_cur->fts_level > FTS_ROOTLEVEL,
              "error: current node retained at non-root level on fatal changedir failure");

    fts_check(fts_close(f) == 0, "error: fts_close succeeds after fatal changedir failure");
}

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    __fts_ops_override = &injected_ops;

    test_prebuilt_children_changedir_fallback(&tree);
    test_parent_changedir_error_stops_walk(&tree);

    __fts_ops_override = NULL;
    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
