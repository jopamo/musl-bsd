#include "fts_test_common.h"

#include <stdlib.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* file_root = fts_join2(tree.abs_root, "file_at_root");
    if (!file_root) {
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    char* roots[] = {file_root, NULL};
    struct fts_walk_stats s;
    fts_run_walk("file root PHYSICAL NOCHDIR", tree.abs_root, roots, FTS_PHYSICAL | FTS_NOCHDIR, false, false, &s);
    fts_check_soft(s.n_files >= 1, "file root produced a file node at level 0");

    free(file_root);
    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
