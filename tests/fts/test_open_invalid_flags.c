#include "fts_test_common.h"

#include <errno.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* roots[] = {tree.abs_root, NULL};

    errno = 0;
    FTS* f = fts_open(roots, 0, NULL);
    fts_check(f == NULL, "fts_open rejects missing LOGICAL/PHYSICAL");
    fts_check(errno == EINVAL, "fts_open missing flags sets errno=EINVAL");
    if (f)
        fts_close(f);

    errno = 0;
    f = fts_open(roots, FTS_LOGICAL | FTS_PHYSICAL, NULL);
    fts_check(f == NULL, "fts_open rejects LOGICAL|PHYSICAL");
    fts_check(errno == EINVAL, "fts_open conflicting flags sets errno=EINVAL");
    if (f)
        fts_close(f);

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
