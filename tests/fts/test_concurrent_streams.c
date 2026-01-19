#include "fts_test_common.h"

#include <string.h>
#include <stdlib.h>

static int path_has_prefix(const char* path, const char* prefix) {
    size_t len = strlen(prefix);
    return strncmp(path, prefix, len) == 0;
}

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* root_a = fts_join2(tree.abs_root, "a");
    char* root_b = fts_join2(tree.abs_root, "b");
    if (!root_a || !root_b) {
        free(root_a);
        free(root_b);
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    char* roots_a[] = {root_a, NULL};
    char* roots_b[] = {root_b, NULL};

    FTS* fa = fts_open(roots_a, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    FTS* fb = fts_open(roots_b, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    fts_check(fa != NULL, "fts_open stream A");
    fts_check(fb != NULL, "fts_open stream B");

    size_t seen_a = 0;
    size_t seen_b = 0;

    if (fa && fb) {
        while (1) {
            FTSENT* ea = fts_read(fa);
            if (ea) {
                seen_a++;
                fts_check(path_has_prefix(ea->fts_path, root_a), "stream A path prefix");
            }

            FTSENT* eb = fts_read(fb);
            if (eb) {
                seen_b++;
                fts_check(path_has_prefix(eb->fts_path, root_b), "stream B path prefix");
            }

            if (!ea && !eb)
                break;
        }
    }

    fts_check(seen_a > 0, "stream A produced entries");
    fts_check(seen_b > 0, "stream B produced entries");

    if (fa)
        fts_close(fa);
    if (fb)
        fts_close(fb);

    free(root_a);
    free(root_b);
    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
