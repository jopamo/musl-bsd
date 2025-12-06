#include "fts_test_common.h"

#include <stdbool.h>
#include <stdio.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    if (fts_build_symlink_loop(tree.abs_root) == -1) {
        perror("build_symlink_loop");
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    char* roots[] = {tree.abs_root, NULL};
    FTS* f = fts_open(roots, FTS_LOGICAL | FTS_NOCHDIR, fts_cmp_asc);
    fts_check(f != NULL, "cycle detection fts_open");
    if (f) {
        bool saw_cycle = false;
        FTSENT* e;
        while ((e = fts_read(f)) != NULL) {
            if (e->fts_info == FTS_DC) {
                saw_cycle = true;
                fts_check(e->fts_cycle != NULL, "cycle entry has fts_cycle pointer");
                if (e->fts_cycle)
                    fts_check(e->fts_cycle->fts_level < e->fts_level, "fts_cycle points to an ancestor node");
            }
        }

        fts_check(saw_cycle, "detected symlink loop between loopA and loopB");
        fts_close(f);
    }

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
