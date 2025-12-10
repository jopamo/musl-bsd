#include "fts_test_common.h"

#include <string.h>

struct child_seen {
    int a;
    int b;
    int dotdir;
    int file;
    int count;
};

static void tally_child(struct child_seen* seen, const char* name) {
    if (!name)
        return;
    if (strcmp(name, "a") == 0)
        seen->a = 1;
    else if (strcmp(name, "b") == 0)
        seen->b = 1;
    else if (strcmp(name, "dotdir") == 0)
        seen->dotdir = 1;
    else if (strcmp(name, "file_at_root") == 0)
        seen->file = 1;
    seen->count++;
}

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
            FTSENT* kids = fts_children(NULL, 0);
            fts_check(kids != NULL, "fts_children(NULL) returns children");

            struct child_seen seen = {0};
            const char* first_name = kids ? kids->fts_name : NULL;
            for (FTSENT* k = kids; k; k = k->fts_link)
                tally_child(&seen, k->fts_name);

            fts_check(seen.count == 4, "saw 4 children at root");
            fts_check(seen.a && seen.b && seen.dotdir && seen.file, "children include a, b, dotdir, file_at_root");

            FTSENT* next = fts_read(f);
            fts_check(next != NULL, "fts_read resumes after fts_children(NULL)");
            if (next && first_name)
                fts_check(strcmp(next->fts_name, first_name) == 0, "cached child list fed traversal");
        }
        fts_close(f);
    }

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
