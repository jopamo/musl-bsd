#include "fts_test_common.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* roots[] = {tree.abs_root, NULL};

    errno = 0;
    FTS* f = fts_open(roots, 0, NULL);
    fts_check(f != NULL, "fts_open accepts missing LOGICAL/PHYSICAL like glibc");
    if (f) {
        FTSENT* e = fts_read(f);
        fts_check(e != NULL, "defaulted traversal produced a root node");
        fts_check(e && e->fts_info == FTS_D, "missing mode defaults to a physical walk");
        fts_close(f);
    }

    char* root_link = fts_join2(tree.root, "root_link");
    fts_check(root_link != NULL, "allocated symlink probe path");
    if (root_link) {
        unlink(root_link);
        fts_check(symlink("a", root_link) == 0, "created root symlink probe");
    }

    char* link_roots[] = {root_link, NULL};
    errno = 0;
    f = root_link ? fts_open(link_roots, FTS_LOGICAL | FTS_PHYSICAL, NULL) : NULL;
    fts_check(f != NULL, "fts_open accepts LOGICAL|PHYSICAL like glibc");
    if (f) {
        FTSENT* e = fts_read(f);
        fts_check(e != NULL, "conflicting-mode traversal produced a root node");
        fts_check(e && e->fts_info == FTS_D, "LOGICAL|PHYSICAL prefers logical traversal");
        fts_close(f);
    }

    free(root_link);

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
