#include "fts_test_common.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* modalias_a = fts_join2(tree.abs_root, "a/modalias");
    char* modalias_b = fts_join2(tree.abs_root, "b/modalias");
    if (!modalias_a || !modalias_b) {
        free(modalias_a);
        free(modalias_b);
        fts_test_tree_cleanup(&tree);
        return 1;
    }
    if (fts_write_file(modalias_a, "alias-a\n") == -1 || fts_write_file(modalias_b, "alias-b\n") == -1) {
        free(modalias_a);
        free(modalias_b);
        fts_test_tree_cleanup(&tree);
        return 1;
    }
    free(modalias_a);
    free(modalias_b);

    char* roots[] = {tree.abs_root, NULL};
    errno = 0;
    FTS* f = fts_open(roots, FTS_NOCHDIR | FTS_NOSTAT, NULL);
    fts_check(f != NULL, "dracut-style fts_open succeeds");

    int modalias_hits = 0;
    if (f) {
        for (FTSENT* e = fts_read(f); e != NULL; e = fts_read(f)) {
            if (strcmp(e->fts_name, "modalias") == 0)
                modalias_hits++;
        }
        fts_check(fts_close(f) == 0, "dracut-style fts_close succeeds");
    }

    fts_check(modalias_hits == 2, "dracut-style walk found nested modalias files");

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
