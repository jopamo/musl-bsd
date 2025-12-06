#include "fts_test_common.h"

#include <stdio.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    if (fts_build_symlink_loop(tree.abs_root) == -1)
        perror("build_symlink_loop");
    else {
        char* roots[] = {tree.abs_root, NULL};
        struct fts_walk_stats s;
        fts_run_walk("symlink loop FOLLOW probe", tree.abs_root, roots, FTS_LOGICAL | FTS_COMFOLLOW | FTS_NOCHDIR, false, true, &s);
        fts_check_soft(s.n_dirs >= 1, "symlink loop traversal completed");
    }

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
