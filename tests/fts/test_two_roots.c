#include "fts_test_common.h"

#include <stdlib.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char** roots = fts_make_roots(tree.rel_b, tree.rel_a);
    struct fts_walk_stats s;
    fts_run_walk("two roots PHYSICAL NOCHDIR", tree.root, roots, FTS_PHYSICAL | FTS_NOCHDIR, true, false, &s);
    free(roots);
    fts_check_soft(s.n_files >= 2, "two roots include files from a and b");

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
