#include "fts_test_common.h"

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* roots[] = {tree.abs_root, NULL};
    struct fts_walk_stats sx;
    fts_run_walk("PHYSICAL XDEV", tree.abs_root, roots, FTS_PHYSICAL | FTS_XDEV, false, false, &sx);
    fts_check_soft(sx.n_dirs >= 1, "XDEV on same device still walks");

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
