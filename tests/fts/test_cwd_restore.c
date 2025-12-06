#include "fts_test_common.h"

#include <limits.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char before[PATH_MAX] = {0};
    char after[PATH_MAX] = {0};

    fts_check_soft(getcwd(before, sizeof(before)) != NULL, "getcwd before");
    {
        char* roots[] = {tree.abs_root, NULL};
        struct fts_walk_stats stats;
        fts_run_walk("cwd restore probe", tree.abs_root, roots, FTS_PHYSICAL, false, false, &stats);
    }
    fts_check_soft(getcwd(after, sizeof(after)) != NULL, "getcwd after");

    if (before[0] && after[0])
        fts_check_soft(strcmp(before, after) == 0, "cwd restored after fts_close");

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
