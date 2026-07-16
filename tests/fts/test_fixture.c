#include "test_support.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int ensure_dir(const char* p, mode_t mode) {
    if (mkdir(p, mode) == -1 && errno != EEXIST)
        return -1;
    return 0;
}

char* fts_join2(const char* a, const char* b) {
    size_t la = strlen(a), lb = strlen(b);
    char* s = (char*)malloc(la + 1 + lb + 1);
    if (!s)
        return NULL;
    memcpy(s, a, la);
    s[la] = '/';
    memcpy(s + la + 1, b, lb + 1);
    return s;
}

int fts_write_file(const char* p, const char* text) {
    int fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
        return -1;
    ssize_t want = (ssize_t)strlen(text);
    ssize_t n = write(fd, text, (size_t)want);
    int ok = (n == want) ? 0 : -1;
    close(fd);
    return ok;
}

static int fts_rm_rf(const char* path) {
    char* const roots[] = {(char*)path, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    if (!f)
        return -1;
    FTSENT* e;
    while ((e = fts_read(f)) != NULL) {
        switch (e->fts_info) {
            case FTS_DP:
                if (rmdir(e->fts_path) == -1 && errno != ENOENT) {
                }
                break;
            default:
                if (unlink(e->fts_path) == -1 && errno != ENOENT) {
                }
                break;
        }
    }
    fts_close(f);
    return 0;
}

static int fts_build_tree(const char* root) {
    char path[PATH_MAX];

    snprintf(path, sizeof(path), "%s/a", root);
    if (ensure_dir(path, 0755) == -1)
        return -1;
    char* f1 = fts_join2(path, "file1");
    if (!f1 || fts_write_file(f1, "hello a\n") == -1) {
        free(f1);
        return -1;
    }
    free(f1);

    char* unread = fts_join2(path, "unreadable_dir");
    if (!unread)
        return -1;
    if (ensure_dir(unread, 0700) == -1) {
        free(unread);
        return -1;
    }
    if (chmod(unread, 0000) == -1) {
        free(unread);
        return -1;
    }
    free(unread);

    snprintf(path, sizeof(path), "%s/b", root);
    if (ensure_dir(path, 0755) == -1)
        return -1;
    char* f2 = fts_join2(path, "file2");
    if (!f2 || fts_write_file(f2, "hello b\n") == -1) {
        free(f2);
        return -1;
    }
    free(f2);

    char* link_to_a = fts_join2(path, "link_to_a");
    if (!link_to_a || symlink("../a", link_to_a) == -1) {
        free(link_to_a);
        return -1;
    }
    free(link_to_a);

    char* link_to_nowhere = fts_join2(path, "link_to_nowhere");
    if (!link_to_nowhere || symlink("./nope", link_to_nowhere) == -1) {
        free(link_to_nowhere);
        return -1;
    }
    free(link_to_nowhere);

    char* cyc = fts_join2(path, "cycle");
    if (!cyc || symlink(".", cyc) == -1) {
        free(cyc);
        return -1;
    }
    free(cyc);

    snprintf(path, sizeof(path), "%s/dotdir", root);
    if (ensure_dir(path, 0755) == -1)
        return -1;
    char* hidden = fts_join2(path, ".hidden");
    if (!hidden || fts_write_file(hidden, "shh\n") == -1) {
        free(hidden);
        return -1;
    }
    free(hidden);

    char* far = fts_join2(root, "file_at_root");
    if (!far || fts_write_file(far, "root file\n") == -1) {
        free(far);
        return -1;
    }
    free(far);

    return 0;
}

int fts_build_many(const char* root, const char* sub, int n) {
    char dir[PATH_MAX];
    snprintf(dir, sizeof(dir), "%s/%s", root, sub);
    if (ensure_dir(dir, 0755) == -1)
        return -1;

    char name[64];
    int width = (n > 0) ? (int)snprintf(NULL, 0, "%d", n - 1) : 1;
    if (width < 1)
        width = 1;
    if (width >= (int)sizeof(name))
        width = (int)sizeof(name) - 1;
    for (int i = 0; i < n; i++) {
        snprintf(name, sizeof(name), "%0*d", width, i);
        char* p = fts_join2(dir, name);
        if (!p)
            return -1;
        if (fts_write_file(p, "x\n") == -1) {
            free(p);
            return -1;
        }
        free(p);
    }
    return 0;
}

int fts_build_symlink_loop(const char* abs_root) {
    char a[PATH_MAX], b[PATH_MAX];
    snprintf(a, sizeof(a), "%s/loopA", abs_root);
    snprintf(b, sizeof(b), "%s/loopB", abs_root);
    if (ensure_dir(a, 0755) == -1)
        return -1;
    if (ensure_dir(b, 0755) == -1)
        return -1;

    char* a_to_b = fts_join2(a, "toB");
    char* b_to_a = fts_join2(b, "toA");
    if (!a_to_b || !b_to_a) {
        free(a_to_b);
        free(b_to_a);
        return -1;
    }

    unlink(a_to_b);
    if (symlink("../loopB", a_to_b) == -1) {
        free(a_to_b);
        free(b_to_a);
        return -1;
    }
    unlink(b_to_a);
    if (symlink("../loopA", b_to_a) == -1) {
        free(a_to_b);
        free(b_to_a);
        return -1;
    }

    free(a_to_b);
    free(b_to_a);
    return 0;
}

int fts_build_order_tree(const char* abs_root) {
    int rc = 0;
    char* base = fts_join2(abs_root, "order");
    char* parent = NULL;
    if (!base)
        return -1;
    if (ensure_dir(base, 0755) == -1) {
        rc = -1;
        goto out;
    }
    parent = fts_join2(base, "parent");
    if (!parent) {
        rc = -1;
        goto out;
    }
    if (ensure_dir(parent, 0755) == -1) {
        rc = -1;
        goto out;
    }
    char* child = fts_join2(parent, "child.txt");
    if (!child || fts_write_file(child, "child\n") == -1) {
        free(child);
        rc = -1;
        goto out;
    }
    free(child);

    char* lone = fts_join2(base, "lone.txt");
    if (!lone || fts_write_file(lone, "lone\n") == -1) {
        free(lone);
        rc = -1;
        goto out;
    }
    free(lone);

    char* broken = fts_join2(base, "broken");
    if (!broken) {
        rc = -1;
        goto out;
    }
    unlink(broken);
    if (symlink("no-such-target", broken) == -1) {
        free(broken);
        rc = -1;
        goto out;
    }
    free(broken);

out:
    free(parent);
    free(base);
    return rc;
}

static void restore_unreadable_dir(const struct fts_test_tree* tree) {
    if (!tree->abs_root)
        return;
    char* unread_abs = fts_join2(tree->abs_root, "a/unreadable_dir");
    if (!unread_abs)
        return;
    chmod(unread_abs, 0700);
    free(unread_abs);
}

int fts_test_tree_init(struct fts_test_tree* tree) {
    memset(tree, 0, sizeof(*tree));
    char tmpl[] = "/tmp/fts-test-XXXXXX";
    char* root = mkdtemp(tmpl);
    if (!root)
        return -1;
    tree->root = strdup(root);
    if (!tree->root)
        goto fail;

    if (fts_build_tree(tree->root) == -1)
        goto fail;

    tree->abs_root = realpath(tree->root, NULL);
    if (!tree->abs_root)
        goto fail;

    tree->rel_a = fts_join2(tree->root, "a");
    tree->rel_b = fts_join2(tree->root, "b");
    if (!tree->rel_a || !tree->rel_b)
        goto fail;

    printf("using temp root %s\n", tree->root);
    return 0;

fail:
    fts_test_tree_cleanup(tree);
    return -1;
}

void fts_test_tree_cleanup(struct fts_test_tree* tree) {
    if (!tree)
        return;
    restore_unreadable_dir(tree);

    if (tree->abs_root)
        fts_rm_rf(tree->abs_root);
    else if (tree->root)
        fts_rm_rf(tree->root);

    free(tree->abs_root);
    free(tree->rel_a);
    free(tree->rel_b);
    free(tree->root);
    memset(tree, 0, sizeof(*tree));
}
