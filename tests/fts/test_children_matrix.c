#include "fts_test_common.h"

#include <errno.h>
#include <string.h>

static void test_invalid_flags_and_init_timing(const struct fts_test_tree* tree) {
    char* roots[] = {tree->abs_root, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    fts_check(f != NULL, "matrix: fts_open succeeds for invalid/init checks");
    if (!f)
        return;

    errno = 0;
    FTSENT* kids = fts_children(f, 42);
    fts_check(kids == NULL, "matrix: invalid children flag is rejected");
    fts_check(errno == EINVAL, "matrix: invalid children flag sets errno=EINVAL");

    errno = 0;
    kids = fts_children(f, 0);
    fts_check(kids != NULL, "matrix: children at FTS_INIT returns root list");
    fts_check(kids && kids->fts_level == FTS_ROOTLEVEL, "matrix: init children entry is at root level");
    fts_check(kids && strcmp(kids->fts_name, tree->abs_root) == 0, "matrix: init children entry names the opened root");

    FTSENT* root = fts_read(f);
    fts_check(root && root->fts_info == FTS_D, "matrix: first read after init-children is root dir");

    fts_check(fts_close(f) == 0, "matrix: close succeeds after invalid/init checks");
}

static void test_nameonly_behavior(const struct fts_test_tree* tree) {
    char* roots[] = {tree->abs_root, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    fts_check(f != NULL, "matrix: fts_open succeeds for name-only checks");
    if (!f)
        return;

    FTSENT* root = fts_read(f);
    fts_check(root && root->fts_info == FTS_D, "matrix: root read succeeds before name-only children");
    if (!root || root->fts_info != FTS_D) {
        fts_close(f);
        return;
    }

    FTSENT* kids = fts_children(f, FTS_NAMEONLY);
    fts_check(kids != NULL, "matrix: FTS_NAMEONLY children list is returned");

    int child_count = 0;
    int all_nsok = 1;
    for (FTSENT* e = kids; e; e = e->fts_link) {
        ++child_count;
        if (e->fts_info != FTS_NSOK)
            all_nsok = 0;
    }

    fts_check(child_count > 0, "matrix: name-only children contains entries");
    fts_check(all_nsok, "matrix: name-only children are marked FTS_NSOK");

    int saw_level1 = 0;
    int saw_level1_nsok = 0;
    FTSENT* e;
    while ((e = fts_read(f)) != NULL) {
        if (e->fts_level == 1 && e->fts_info != FTS_DP) {
            saw_level1 = 1;
            if (e->fts_info == FTS_NSOK)
                saw_level1_nsok = 1;
        }
    }

    fts_check(saw_level1, "matrix: traversal reached first-level children after name-only call");
    fts_check(!saw_level1_nsok, "matrix: fts_read rebuilt children with full stat info");

    fts_check(fts_close(f) == 0, "matrix: close succeeds after name-only checks");
}

static void test_timing_postorder_and_exhausted(const struct fts_test_tree* tree) {
    char* roots[] = {tree->abs_root, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    fts_check(f != NULL, "matrix: fts_open succeeds for timing checks");
    if (!f)
        return;

    int saw_root_postorder = 0;
    FTSENT* e;
    while ((e = fts_read(f)) != NULL) {
        if (e->fts_level == FTS_ROOTLEVEL && e->fts_info == FTS_DP) {
            saw_root_postorder = 1;
            errno = E2BIG;
            FTSENT* kids = fts_children(f, 0);
            fts_check(kids == NULL, "matrix: children on postorder node returns NULL");
            fts_check(errno == 0, "matrix: children on postorder node clears errno to 0");
        }
    }

    fts_check(saw_root_postorder, "matrix: traversal reached root postorder state");

    errno = E2BIG;
    FTSENT* kids = fts_children(f, 0);
    fts_check(kids == NULL, "matrix: children after traversal exhaustion returns NULL");
    fts_check(errno == E2BIG, "matrix: exhausted children call leaves errno untouched");

    fts_check(fts_close(f) == 0, "matrix: close succeeds after timing checks");
}

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    test_invalid_flags_and_init_timing(&tree);
    test_nameonly_behavior(&tree);
    test_timing_postorder_and_exhausted(&tree);

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
