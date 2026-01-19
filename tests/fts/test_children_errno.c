#include "fts_test_common.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* empty_dir = fts_join2(tree.abs_root, "empty");
    if (!empty_dir) {
        fts_test_tree_cleanup(&tree);
        return 1;
    }
    if (mkdir(empty_dir, 0755) == -1) {
        free(empty_dir);
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    char* roots[] = {tree.abs_root, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    fts_check(f != NULL, "fts_open for children errno tests");

    int saw_file = 0;
    int saw_empty = 0;
    if (f) {
        FTSENT* e;
        while ((e = fts_read(f)) != NULL) {
            if (e->fts_info == FTS_F && strcmp(e->fts_name, "file_at_root") == 0) {
                errno = E2BIG;
                FTSENT* kids = fts_children(f, 0);
                fts_check(kids == NULL, "fts_children on file returns NULL");
                fts_check(errno == 0, "fts_children on file sets errno=0");
                saw_file = 1;
            }
            if (e->fts_info == FTS_D && strcmp(e->fts_name, "empty") == 0) {
                errno = E2BIG;
                FTSENT* kids = fts_children(f, 0);
                fts_check(kids == NULL, "fts_children on empty dir returns NULL");
                fts_check(errno == 0, "fts_children on empty dir sets errno=0");
                saw_empty = 1;
            }
        }
        fts_close(f);
    }

    fts_check(saw_file, "saw file_at_root during traversal");
    fts_check(saw_empty, "saw empty directory during traversal");

    free(empty_dir);
    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
