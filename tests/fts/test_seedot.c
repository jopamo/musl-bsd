#include "fts_test_common.h"

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* roots[] = {tree.abs_root, NULL};
    struct fts_walk_stats s;
    fts_run_walk("SEEDOT probe", tree.abs_root, roots, FTS_PHYSICAL | FTS_SEEDOT | FTS_NOCHDIR, false, false, &s);
    fts_check_soft(s.n_dirs >= 1, "SEEDOT traversal ok");

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
