#include "fts_test_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    if (fts_build_many(tree.abs_root, "many", 2000) == -1) {
        perror("build_many");
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    char* many = fts_join2(tree.abs_root, "many");
    if (!many) {
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    char* roots[] = {many, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, fts_cmp_rev);
    fts_check_soft(f != NULL, "fts_open many children");
    if (f) {
        FTSENT* e;
        const char* first = NULL;
        const char* second = NULL;
        int seen = 0;
        while ((e = fts_read(f)) != NULL) {
            if (e->fts_level == 1) {
                if (seen == 0)
                    first = strdup(e->fts_name);
                else if (seen == 1)
                    second = strdup(e->fts_name);
                seen++;
                if (seen >= 2)
                    break;
            }
        }
        fts_close(f);

        if (first && second)
            fts_check_soft(strcmp(first, second) > 0, "reverse comparator affects sibling order");
        else
            fts_check_soft(false, "failed to capture sibling ordering in many children dir");

        free((void*)first);
        free((void*)second);
    }

    free(many);
    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
