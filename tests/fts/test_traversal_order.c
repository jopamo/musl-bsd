#include "fts_test_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    if (fts_build_order_tree(tree.abs_root) == -1) {
        perror("build_order_tree");
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    char* order_root = fts_join2(tree.abs_root, "order");
    if (!order_root) {
        fts_test_tree_cleanup(&tree);
        return 1;
    }

    char* roots[] = {order_root, NULL};
    FTS* f = fts_open(roots, FTS_LOGICAL | FTS_NOCHDIR, fts_cmp_asc);
    fts_check(f != NULL, "order traversal fts_open");
    if (f) {
        struct expected {
            const char* name;
            int info;
            int level;
        } exp[] = {
            {"order", FTS_D, FTS_ROOTLEVEL},
            {"broken", FTS_SLNONE, 1},
            {"lone.txt", FTS_F, 1},
            {"parent", FTS_D, 1},
            {"child.txt", FTS_F, 2},
            {"parent", FTS_DP, 1},
            {"order", FTS_DP, 0},
        };
        const size_t expected_len = sizeof(exp) / sizeof(exp[0]);

        size_t idx = 0;
        FTSENT* e;
        while ((e = fts_read(f)) != NULL) {
            if (idx >= expected_len) {
                fts_check(false, "order traversal produced unexpected extra entry \"%s\"", e->fts_name);
                break;
            }
            fts_check(strcmp(e->fts_name, exp[idx].name) == 0, "order traversal entry %zu name=%s", idx, e->fts_name);
            fts_check(e->fts_info == exp[idx].info,
                      "order traversal entry %zu info=%s (wanted %s)",
                      idx,
                      fts_info_label(e->fts_info),
                      fts_info_label(exp[idx].info));
            fts_check(e->fts_level == exp[idx].level,
                      "order traversal entry %zu level=%d expected=%d",
                      idx,
                      e->fts_level,
                      exp[idx].level);
            idx++;
        }
        fts_check(idx == expected_len, "order traversal visited expected number of nodes (%zu)", idx);
        fts_check(e == NULL, "order traversal ended cleanly");
        fts_close(f);
    }

    free(order_root);
    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
