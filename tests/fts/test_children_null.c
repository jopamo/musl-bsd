#include "fts_test_common.h"

#include <errno.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* roots[] = {tree.abs_root, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    fts_check(f != NULL, "fts_open");
    if (f) {
        FTSENT* e = fts_read(f);
        fts_check(e && e->fts_info == FTS_D, "first entry is root directory");
        if (e && e->fts_info == FTS_D) {
            errno = 0;
            FTSENT* kids = fts_children(NULL, 0);
            fts_check(kids == NULL, "fts_children(NULL) is rejected");
            fts_check(errno == EINVAL, "fts_children(NULL) sets errno=EINVAL");
        }
        fts_close(f);
    }

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
