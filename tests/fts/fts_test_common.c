#include "fts_test_common.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define C_RED "\033[31m"
#define C_GRN "\033[32m"
#define C_YEL "\033[33m"
#define C_BLU "\033[34m"
#define C_RST "\033[0m"

int fts_test_failures = 0;
int fts_test_strict = 0;

static void vcheck_log(int cond, int hard, const char* fmt, va_list ap) {
    const char* color = cond ? C_GRN : (hard ? C_RED : C_YEL);
    const char* tag = cond ? "[ ok ]" : (hard ? "[fail]" : "[warn]");

    fprintf(stderr, "%s%s " C_RST, color, tag);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

void fts_set_strict_from_env(void) {
    const char* env = getenv("FTS_TEST_STRICT");
    fts_test_strict = env && strcmp(env, "0") != 0;
    fts_test_failures = 0;
}

void fts_check(int cond, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vcheck_log(cond, 1, fmt, ap);
    va_end(ap);
    if (!cond)
        fts_test_failures++;
}

void fts_check_soft(int cond, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vcheck_log(cond, fts_test_strict, fmt, ap);
    va_end(ap);
    if (!cond && fts_test_strict)
        fts_test_failures++;
}

int fts_exit_code(void) {
    if (fts_test_failures) {
        fprintf(stderr, C_RED "\n%d checks failed\n" C_RST, fts_test_failures);
        return 1;
    }
    fprintf(stderr, C_GRN "\nall checks passed\n" C_RST);
    return 0;
}

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

int fts_rm_rf(const char* path) {
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

int fts_build_tree(const char* root) {
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

static ssize_t readlink_buf(const char* p, char* buf, size_t sz) {
    ssize_t n = readlink(p, buf, sz - 1);
    if (n >= 0)
        buf[n] = '\0';
    return n;
}

const char* fts_info_label(int info) {
    switch (info) {
        case FTS_D:
            return "FTS_D";
        case FTS_DC:
            return "FTS_DC";
        case FTS_DEFAULT:
            return "FTS_DEFAULT";
        case FTS_DNR:
            return "FTS_DNR";
        case FTS_ERR:
            return "FTS_ERR";
        case FTS_F:
            return "FTS_F";
        case FTS_INIT:
            return "FTS_INIT";
        case FTS_NS:
            return "FTS_NS";
        case FTS_NSOK:
            return "FTS_NSOK";
        case FTS_SL:
            return "FTS_SL";
        case FTS_SLNONE:
            return "FTS_SLNONE";
        case FTS_DP:
            return "FTS_DP";
        case FTS_DOT:
            return "FTS_DOT";
        default:
            return "FTS_?";
    }
}

void fts_print_ent(const char* root_prefix, const FTSENT* e) {
    char full[PATH_MAX];
    if (root_prefix && e->fts_path[0] != '/')
        snprintf(full, sizeof(full), "%s/%s", root_prefix, e->fts_path);
    else
        snprintf(full, sizeof(full), "%s", e->fts_path);

    struct stat st;
    int lst_ok = lstat(full, &st);
    char lnk[PATH_MAX];
    lnk[0] = '\0';
    if (lst_ok == 0 && S_ISLNK(st.st_mode)) {
        if (readlink_buf(full, lnk, sizeof(lnk)) < 0)
            snprintf(lnk, sizeof(lnk), "<readlink err:%s>", strerror(errno));
    }

    printf("level=%-3d info=%-9s name=\"%s\" path=\"%s\" acc=\"%s\" full=\"%s\"",
           e->fts_level,
           fts_info_label(e->fts_info),
           e->fts_name,
           e->fts_path,
           e->fts_accpath ? e->fts_accpath : "",
           full);
    if (lnk[0])
        printf(" -> \"%s\"", lnk);
    printf("\n");
}

int fts_cmp_rev(const FTSENT** a, const FTSENT** b) {
    return -strcmp((*a)->fts_name, (*b)->fts_name);
}

int fts_cmp_asc(const FTSENT** a, const FTSENT** b) {
    return strcmp((*a)->fts_name, (*b)->fts_name);
}

void fts_list_children(FTS* fts, int nameonly) {
    FTSENT* kid = fts_children(fts, nameonly ? FTS_NAMEONLY : 0);
    if (!kid) {
        printf("children%s: <none>\n", nameonly ? " nameonly" : "");
        return;
    }
    printf("children%s:\n", nameonly ? " nameonly" : "");
    for (FTSENT* e = kid; e; e = e->fts_link)
        printf("  child name=\"%s\" info=%s\n", e->fts_name, fts_info_label(e->fts_info));
}

void fts_reset_stats(struct fts_walk_stats* s) {
    memset(s, 0, sizeof(*s));
}

void fts_account(struct fts_walk_stats* s, const FTSENT* e) {
    switch (e->fts_info) {
        case FTS_D:
            s->n_dirs++;
            break;
        case FTS_DP:
            s->n_post++;
            break;
        case FTS_F:
            s->n_files++;
            break;
        case FTS_SL:
            s->n_symlinks++;
            break;
        case FTS_SLNONE:
            s->n_dangling++;
            break;
        case FTS_DNR:
        case FTS_ERR:
        case FTS_NS:
            s->n_errors++;
            break;
        default:
            break;
    }
}

void fts_run_walk(const char* label,
                  const char* root_prefix,
                  char* const* roots,
                  int opts,
                  bool use_cmp,
                  bool try_instr,
                  struct fts_walk_stats* out) {
    printf(C_BLU "\n== %s ==\n" C_RST, label);
    printf("options:");
    if (opts & FTS_NOCHDIR)
        printf(" FTS_NOCHDIR");
    if (opts & FTS_PHYSICAL)
        printf(" FTS_PHYSICAL");
    if (opts & FTS_LOGICAL)
        printf(" FTS_LOGICAL");
    if (opts & FTS_COMFOLLOW)
        printf(" FTS_COMFOLLOW");
    if (opts & FTS_SEEDOT)
        printf(" FTS_SEEDOT");
    if (opts & FTS_NOSTAT)
        printf(" FTS_NOSTAT");
    if (opts & FTS_XDEV)
        printf(" FTS_XDEV");
    printf("\n");

    errno = 0;
    FTS* f = fts_open(roots, opts, use_cmp ? fts_cmp_rev : NULL);
    fts_check_soft(f != NULL, "fts_open %s", label);
    if (!f) {
        fprintf(stderr, "fts_open errno=%d %s\n", errno, strerror(errno));
        return;
    }

    fts_reset_stats(out);
    int again_set = 0;
    FTSENT* e;
    while ((e = fts_read(f)) != NULL) {
        fts_account(out, e);
        fts_print_ent(root_prefix, e);

        if (e->fts_info == FTS_D && strcmp(e->fts_name, "b") == 0 && e->fts_level >= 1) {
            fts_list_children(f, 1);
            fts_list_children(f, 0);
        }
        if (try_instr && e->fts_info == FTS_D && strcmp(e->fts_name, "a") == 0) {
            printf("setting SKIP on dir a\n");
            fts_set(f, e, FTS_SKIP);
        }
        if (try_instr && e->fts_info == FTS_SL && strcmp(e->fts_name, "cycle") == 0) {
            printf("setting FOLLOW on symlink cycle\n");
            fts_set(f, e, FTS_FOLLOW);
        }
        if (try_instr && !again_set && e->fts_info == FTS_F && strcmp(e->fts_name, "file_at_root") == 0) {
            printf("setting AGAIN on %s\n", e->fts_name);
            fts_set(f, e, FTS_AGAIN);
            again_set = 1;
        }
    }

    int saved = errno;
    fts_check_soft(fts_close(f) == 0, "fts_close %s", label);

    if (saved != 0 && saved != ENOENT)
        fprintf(stderr, C_YEL "[warn] " C_RST "fts_read ended with errno=%d %s\n", saved, strerror(saved));
}

char** fts_make_roots(const char* a, const char* b) {
    size_t n = (a ? 1 : 0) + (b ? 1 : 0);
    char** v = (char**)calloc(n + 1, sizeof(char*));
    if (!v)
        return NULL;
    size_t i = 0;
    if (a)
        v[i++] = (char*)a;
    if (b)
        v[i++] = (char*)b;
    v[i] = NULL;
    return v;
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
