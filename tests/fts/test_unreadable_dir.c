#include "fts_test_common.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* unread_abs = fts_join2(tree.abs_root, "a/unreadable_dir");
    if (unread_abs) {
        char* roots[] = {unread_abs, NULL};
        FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
        if (f) {
            bool saw_err = false;
            FTSENT* e;
            while ((e = fts_read(f)) != NULL) {
                if (e->fts_info == FTS_DNR || e->fts_info == FTS_ERR) {
                    saw_err = true;
                    fts_print_ent(tree.abs_root, e);
                }
            }
            fts_check_soft(saw_err, "unreadable directory produced an error surface");
            fts_close(f);
        }
        else {
            perror("fts_open unreadable");
        }
        free(unread_abs);
    }

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
