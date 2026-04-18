#include "fts_test_common.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum {
    CYCLE_BUCKETS = 64,
    MAX_CHAIN_DEPTH = 96,
};

static size_t cycle_bucket(dev_t dev, ino_t ino) {
    size_t h = ((size_t)dev << 5) ^ (size_t)ino;
    return h & (CYCLE_BUCKETS - 1);
}

static int stat_dev_ino(const char* path, dev_t* dev, ino_t* ino) {
    struct stat st;
    if (stat(path, &st) == -1)
        return -1;
    *dev = st.st_dev;
    *ino = st.st_ino;
    return 0;
}

int main(void) {
    fts_set_strict_from_env();

    struct fts_test_tree tree;
    if (fts_test_tree_init(&tree) == -1)
        return 1;

    char* chain[MAX_CHAIN_DEPTH + 1] = {0};
    dev_t devs[MAX_CHAIN_DEPTH + 1] = {0};
    ino_t inos[MAX_CHAIN_DEPTH + 1] = {0};
    int first_seen[CYCLE_BUCKETS];
    for (int i = 0; i < CYCLE_BUCKETS; ++i)
        first_seen[i] = -1;

    int last_depth = -1;
    int anchor_depth = -1;
    int collision_depth = -1;
    char* backlink = NULL;

    chain[0] = fts_join2(tree.abs_root, "cycle-collision");
    fts_check(chain[0] != NULL, "cycle-table: allocate collision root path");
    if (chain[0]) {
        if (mkdir(chain[0], 0755) == -1)
            fts_check(0, "cycle-table: mkdir collision root");
        else if (stat_dev_ino(chain[0], &devs[0], &inos[0]) == -1)
            fts_check(0, "cycle-table: stat collision root");
        else {
            first_seen[cycle_bucket(devs[0], inos[0])] = 0;
            last_depth = 0;
        }
    }

    for (int depth = 1; depth <= MAX_CHAIN_DEPTH && collision_depth == -1; ++depth) {
        if (!chain[depth - 1])
            break;

        char name[16];
        snprintf(name, sizeof(name), "d%03d", depth);
        chain[depth] = fts_join2(chain[depth - 1], name);
        fts_check(chain[depth] != NULL, "cycle-table: allocate chain path depth=%d", depth);
        if (!chain[depth])
            break;

        if (mkdir(chain[depth], 0755) == -1) {
            fts_check(0, "cycle-table: mkdir chain depth=%d", depth);
            break;
        }

        if (stat_dev_ino(chain[depth], &devs[depth], &inos[depth]) == -1) {
            fts_check(0, "cycle-table: stat chain depth=%d", depth);
            break;
        }

        size_t idx = cycle_bucket(devs[depth], inos[depth]);
        if (first_seen[idx] != -1) {
            anchor_depth = first_seen[idx];
            collision_depth = depth;
        }
        else {
            first_seen[idx] = depth;
        }

        last_depth = depth;
    }

    fts_check(collision_depth != -1, "cycle-table: found hash-bucket collision across active directories");

    if (collision_depth != -1 && chain[collision_depth]) {
        backlink = fts_join2(chain[collision_depth], "back_to_anchor");
        fts_check(backlink != NULL, "cycle-table: allocate backlink path");
        if (backlink) {
            int rc = symlink(chain[anchor_depth], backlink);
            fts_check(rc == 0, "cycle-table: create backlink to colliding anchor");
        }
    }

    if (collision_depth != -1 && chain[0] && backlink) {
        char* roots[] = {chain[0], NULL};
        FTS* f = fts_open(roots, FTS_LOGICAL | FTS_NOCHDIR, NULL);
        fts_check(f != NULL, "cycle-table: fts_open succeeds");

        if (f) {
            bool saw_anchor_dir = false;
            bool saw_collision_dir = false;
            bool saw_cycle_entry = false;

            FTSENT* e;
            while ((e = fts_read(f)) != NULL) {
                if (e->fts_info == FTS_D && e->fts_level == anchor_depth && e->fts_dev == devs[anchor_depth] &&
                    e->fts_ino == inos[anchor_depth]) {
                    saw_anchor_dir = true;
                }

                if (e->fts_level == collision_depth && e->fts_info != FTS_DP && e->fts_dev == devs[collision_depth] &&
                    e->fts_ino == inos[collision_depth]) {
                    saw_collision_dir = true;
                    fts_check(e->fts_info == FTS_D, "cycle-table: colliding directory is not misreported as a cycle");
                    if (e->fts_info == FTS_D) {
                        fts_check(e->fts_cycle == NULL,
                                  "cycle-table: colliding directory remains non-cycle (no false positive)");
                    }
                }

                if (strcmp(e->fts_name, "back_to_anchor") == 0) {
                    saw_cycle_entry = true;
                    fts_check(e->fts_info == FTS_DC, "cycle-table: backlink is reported as FTS_DC");
                    fts_check(e->fts_cycle != NULL, "cycle-table: backlink carries cycle pointer");
                    if (e->fts_cycle) {
                        fts_check(
                            e->fts_cycle->fts_dev == devs[anchor_depth] && e->fts_cycle->fts_ino == inos[anchor_depth],
                            "cycle-table: cycle pointer resolves to selected colliding anchor");
                    }
                }
            }

            fts_check(saw_anchor_dir, "cycle-table: visited anchor directory");
            fts_check(saw_collision_dir, "cycle-table: visited colliding descendant directory");
            fts_check(saw_cycle_entry, "cycle-table: visited backlink cycle entry");
            fts_check(fts_close(f) == 0, "cycle-table: fts_close succeeds");
        }
    }

    free(backlink);
    for (int i = 0; i <= last_depth; ++i)
        free(chain[i]);

    fts_test_tree_cleanup(&tree);
    return fts_exit_code();
}
