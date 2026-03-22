#include "fts_test_common.h"

#include <stdlib.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char** roots = fts_make_roots(tree.abs_root, NULL);
    struct fts_walk_stats s;
    fts_run_walk("PHYSICAL with chdir", tree.abs_root, roots, FTS_PHYSICAL, true, true, &s);
    free(roots);
    fts_check_soft(s.n_dirs >= 4, "visited nested directories under an absolute root");
    fts_check_soft(s.n_files >= 3, "visited regular files under chdir traversal");

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
