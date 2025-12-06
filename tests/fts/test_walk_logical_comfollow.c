#include "fts_test_common.h"

#include <stdlib.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char** roots = fts_make_roots(tree.abs_root, NULL);
    struct fts_walk_stats s;
    fts_run_walk("LOGICAL with COMFOLLOW", tree.abs_root, roots, FTS_LOGICAL | FTS_COMFOLLOW, false, true, &s);
    free(roots);
    fts_check_soft(s.n_dirs >= 3, "following symlinks did not regress directories");

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
