#include "fts_test_common.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* roots_one[] = {tree.abs_root, NULL};
    FTS* f = fts_open(roots_one, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    fts_check_soft(f != NULL, "children smoke open");
    if (f) {
        FTSENT* e;
        while ((e = fts_read(f)) != NULL) {
            if (e->fts_level == 1 && e->fts_info == FTS_D && strcmp(e->fts_name, "b") == 0) {
                printf("children at %s\n", e->fts_path);
                fts_list_children(f, 1);
                fts_list_children(f, 0);
                break;
            }
        }
        fts_close(f);
    }

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
