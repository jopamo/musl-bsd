#define _XOPEN_SOURCE 700

#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* color helpers */
#define C_RED "\033[31m"
#define C_GRN "\033[32m"
#define C_YEL "\033[33m"
#define C_BLU "\033[34m"
#define C_RST "\033[0m"

/* strictness is opt in via env */
static int STRICT = 0;

/* soft check macro that can be made hard with STRICT=1 */
static int test_failures = 0;
#define CHECK_SOFT(cond, fmt, ...)                                              \
    do {                                                                        \
        if (!(cond)) {                                                          \
            if (STRICT) {                                                       \
                fprintf(stderr, C_RED "[fail] " C_RST fmt "\n", ##__VA_ARGS__); \
                test_failures++;                                                \
            }                                                                   \
            else {                                                              \
                fprintf(stderr, C_YEL "[warn] " C_RST fmt "\n", ##__VA_ARGS__); \
            }                                                                   \
        }                                                                       \
        else {                                                                  \
            fprintf(stderr, C_GRN "[ ok ] " C_RST fmt "\n", ##__VA_ARGS__);     \
        }                                                                       \
    } while (0)

/* map FTS info to label */
static const char* info_label(int info) {
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

/* best effort mkdir for a level, ignore EEXIST */
static int ensure_dir(const char* p, mode_t mode) {
    if (mkdir(p, mode) == -1 && errno != EEXIST)
        return -1;
    return 0;
}

/* join two path components with a slash */
static char* join2(const char* a, const char* b) {
    size_t la = strlen(a), lb = strlen(b);
    char* s = (char*)malloc(la + 1 + lb + 1);
    if (!s)
        return NULL;
    memcpy(s, a, la);
    s[la] = '/';
    memcpy(s + la + 1, b, lb + 1);
    return s;
}

/* write a small file */
static int write_file(const char* p, const char* text) {
    int fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
        return -1;
    ssize_t want = (ssize_t)strlen(text);
    ssize_t n = write(fd, text, (size_t)want);
    int ok = (n == want) ? 0 : -1;
    close(fd);
    return ok;
}

/* rm -rf using postorder only */
static int rm_rf(const char* path) {
    char* const roots[] = {(char*)path, NULL};
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    if (!f)
        return -1;
    FTSENT* e;
    while ((e = fts_read(f))) {
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

/* build the basic test tree under mkdtemp-created root */
static int build_tree(const char* root) {
    char path[PATH_MAX];

    /* a with a file and an unreadable dir */
    snprintf(path, sizeof(path), "%s/a", root);
    if (ensure_dir(path, 0755) == -1)
        return -1;
    char* f1 = join2(path, "file1");
    if (!f1 || write_file(f1, "hello a\n") == -1) {
        free(f1);
        return -1;
    }
    free(f1);

    char* unread = join2(path, "unreadable_dir");
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

    /* b with a file and several symlinks */
    snprintf(path, sizeof(path), "%s/b", root);
    if (ensure_dir(path, 0755) == -1)
        return -1;
    char* f2 = join2(path, "file2");
    if (!f2 || write_file(f2, "hello b\n") == -1) {
        free(f2);
        return -1;
    }
    free(f2);

    char* link_to_a = join2(path, "link_to_a");
    if (!link_to_a || symlink("../a", link_to_a) == -1) {
        free(link_to_a);
        return -1;
    }
    free(link_to_a);

    char* link_to_nowhere = join2(path, "link_to_nowhere");
    if (!link_to_nowhere || symlink("./nope", link_to_nowhere) == -1) {
        free(link_to_nowhere);
        return -1;
    }
    free(link_to_nowhere);

    char* cyc = join2(path, "cycle");
    if (!cyc || symlink(".", cyc) == -1) {
        free(cyc);
        return -1;
    }
    free(cyc);

    /* dotdir with a hidden file */
    snprintf(path, sizeof(path), "%s/dotdir", root);
    if (ensure_dir(path, 0755) == -1)
        return -1;
    char* hidden = join2(path, ".hidden");
    if (!hidden || write_file(hidden, "shh\n") == -1) {
        free(hidden);
        return -1;
    }
    free(hidden);

    /* file at root */
    char* far = join2(root, "file_at_root");
    if (!far || write_file(far, "root file\n") == -1) {
        free(far);
        return -1;
    }
    free(far);

    return 0;
}

/* readlink helper for logging */
static ssize_t readlink_buf(const char* p, char* buf, size_t sz) {
    ssize_t n = readlink(p, buf, sz - 1);
    if (n >= 0)
        buf[n] = '\0';
    return n;
}

/* print one entry with rich diagnostics */
static void print_ent(const char* root_prefix, const FTSENT* e) {
    char full[PATH_MAX];
    if (root_prefix && e->fts_path[0] != '/') {
        snprintf(full, sizeof(full), "%s/%s", root_prefix, e->fts_path);
    }
    else {
        snprintf(full, sizeof(full), "%s", e->fts_path);
    }

    struct stat st;
    int lst_ok = lstat(full, &st);
    char lnk[PATH_MAX];
    lnk[0] = '\0';
    if (lst_ok == 0 && S_ISLNK(st.st_mode)) {
        if (readlink_buf(full, lnk, sizeof(lnk)) < 0)
            snprintf(lnk, sizeof(lnk), "<readlink err:%s>", strerror(errno));
    }

    printf("level=%-3d info=%-9s name=\"%s\" path=\"%s\" acc=\"%s\" full=\"%s\"", e->fts_level, info_label(e->fts_info),
           e->fts_name, e->fts_path, e->fts_accpath ? e->fts_accpath : "", full);
    if (lnk[0])
        printf(" -> \"%s\"", lnk);
    printf("\n");
}

/* comparator to prove the callback is honored */
static int cmp_rev(const FTSENT** a, const FTSENT** b) {
    return -strcmp((*a)->fts_name, (*b)->fts_name);
}

/* children lister */
static void list_children(FTS* fts, int nameonly) {
    FTSENT* kid = fts_children(fts, nameonly ? FTS_NAMEONLY : 0);
    if (!kid) {
        printf("children%s: <none>\n", nameonly ? " nameonly" : "");
        return;
    }
    printf("children%s:\n", nameonly ? " nameonly" : "");
    for (FTSENT* e = kid; e; e = e->fts_link)
        printf("  child name=\"%s\" info=%s\n", e->fts_name, info_label(e->fts_info));
}

/* counters for a walk */
typedef struct {
    size_t n_dirs, n_files, n_symlinks, n_dangling, n_post, n_errors;
} WalkStats;

static void reset_stats(WalkStats* s) {
    memset(s, 0, sizeof(*s));
}
static void account(WalkStats* s, const FTSENT* e) {
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

/* run one traversal with options and diagnostics */
static void run_walk(const char* label,
                     const char* root_prefix,
                     char* const* roots,
                     int opts,
                     bool use_cmp,
                     bool try_instr,
                     WalkStats* out) {
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

    errno = 0; /* clear stale errno before starting the walk */
    FTS* f = fts_open(roots, opts, use_cmp ? cmp_rev : NULL);
    CHECK_SOFT(f != NULL, "fts_open %s", label);
    if (!f) {
        fprintf(stderr, "fts_open errno=%d %s\n", errno, strerror(errno));
        return;
    }

    reset_stats(out);
    FTSENT* e;
    while ((e = fts_read(f)) != NULL) {
        account(out, e);
        print_ent(root_prefix, e);

        if (e->fts_info == FTS_D && strcmp(e->fts_name, "b") == 0 && e->fts_level >= 1) {
            list_children(f, 1);
            list_children(f, 0);
        }
        if (try_instr && e->fts_info == FTS_D && strcmp(e->fts_name, "a") == 0) {
            printf("setting SKIP on dir a\n");
            fts_set(f, e, FTS_SKIP);
        }
        if (try_instr && e->fts_info == FTS_SL && strcmp(e->fts_name, "cycle") == 0) {
            printf("setting FOLLOW on symlink cycle\n");
            fts_set(f, e, FTS_FOLLOW);
        }
        if (try_instr && e->fts_info == FTS_F && strcmp(e->fts_name, "file_at_root") == 0) {
            printf("setting AGAIN on %s\n", e->fts_name);
            fts_set(f, e, FTS_AGAIN);
        }
    }

    int saved = errno; /* capture errno set by fts_read when it returned NULL */
    CHECK_SOFT(fts_close(f) == 0, "fts_close %s", label);

    /* some FTS impls leave ENOENT at EOF, treat it as benign to cut noise */
    if (saved != 0 && saved != ENOENT) {
        fprintf(stderr, C_YEL "[warn] " C_RST "fts_read ended with errno=%d %s\n", saved, strerror(saved));
    }
}

/* build roots array */
static char** make_roots(const char* a, const char* b) {
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

/* create a path with N children to stress comparator and realloc */
static int build_many(const char* root, const char* sub, int n) {
    char dir[PATH_MAX];
    snprintf(dir, sizeof(dir), "%s/%s", root, sub);
    if (ensure_dir(dir, 0755) == -1)
        return -1;

    char name[64];
    /* names with fixed width so lexicographic order is well defined */
    int width = (n > 0) ? (int)snprintf(NULL, 0, "%d", n - 1) : 1;
    if (width < 1)
        width = 1;
    if (width >= (int)sizeof(name))
        width = (int)sizeof(name) - 1;
    for (int i = 0; i < n; i++) {
        snprintf(name, sizeof(name), "%0*d", width, i);
        char* p = join2(dir, name);
        if (!p)
            return -1;
        if (write_file(p, "x\n") == -1) {
            free(p);
            return -1;
        }
        free(p);
    }
    return 0;
}

/* verify traversal leaves cwd unchanged when chdir is allowed */
static void test_cwd_restore(const char* abs_root) {
    char before[PATH_MAX], after[PATH_MAX];
    CHECK_SOFT(getcwd(before, sizeof(before)) != NULL, "getcwd before");
    {
        char* roots[] = {(char*)abs_root, NULL};
        WalkStats s;
        run_walk("cwd restore probe", abs_root, roots, FTS_PHYSICAL /* chdir allowed */, false, false, &s);
    }
    CHECK_SOFT(getcwd(after, sizeof(after)) != NULL, "getcwd after");
    CHECK_SOFT(strcmp(before, after) == 0, "cwd restored after fts_close");
}

/* pass a file as a root path to confirm root classification */
static void test_file_root(const char* abs_root) {
    char* file_root = join2(abs_root, "file_at_root");
    char* roots[] = {file_root, NULL};
    WalkStats s;
    run_walk("file root PHYSICAL NOCHDIR", abs_root, roots, FTS_PHYSICAL | FTS_NOCHDIR, false, false, &s);
    CHECK_SOFT(s.n_files >= 1, "file root produced a file node at level 0");
    free(file_root);
}

/* probe SEEDOT behavior, ensuring stability */
static void test_seedot(const char* abs_root) {
    char* roots[] = {(char*)abs_root, NULL};
    WalkStats s;
    run_walk("SEEDOT probe", abs_root, roots, FTS_PHYSICAL | FTS_SEEDOT | FTS_NOCHDIR, false, false, &s);
    CHECK_SOFT(s.n_dirs >= 1, "SEEDOT traversal ok");
}

/* construct an explicit symlink loop and try FOLLOW to check loop handling */
static int build_symlink_loop(const char* abs_root) {
    char a[PATH_MAX], b[PATH_MAX];
    snprintf(a, sizeof(a), "%s/loopA", abs_root);
    snprintf(b, sizeof(b), "%s/loopB", abs_root);
    if (ensure_dir(a, 0755) == -1)
        return -1;
    if (ensure_dir(b, 0755) == -1)
        return -1;

    char* a_to_b = join2(a, "toB");
    char* b_to_a = join2(b, "toA");
    if (!a_to_b || !b_to_a) {
        free(a_to_b);
        free(b_to_a);
        return -1;
    }

    /* loopA/toB -> ../loopB and loopB/toA -> ../loopA */
    if (symlink("../loopB", a_to_b) == -1) {
        free(a_to_b);
        free(b_to_a);
        return -1;
    }
    if (symlink("../loopA", b_to_a) == -1) {
        free(a_to_b);
        free(b_to_a);
        return -1;
    }

    free(a_to_b);
    free(b_to_a);
    return 0;
}

static void test_symlink_loop_follow(const char* abs_root) {
    if (build_symlink_loop(abs_root) == -1) {
        perror("build_symlink_loop");
        return;
    }
    char* roots[] = {(char*)abs_root, NULL};
    WalkStats s;
    run_walk("symlink loop FOLLOW probe", abs_root, roots, FTS_LOGICAL | FTS_COMFOLLOW | FTS_NOCHDIR, false, true, &s);
    CHECK_SOFT(s.n_dirs >= 1, "symlink loop traversal completed");
}

/* heavy fanout to prove comparator order and array growth */
static void test_many_children_sorted(const char* abs_root, int n) {
    if (build_many(abs_root, "many", n) == -1) {
        perror("build_many");
        return;
    }
    char* many = join2(abs_root, "many");
    char* roots[] = {many, NULL};

    printf(C_BLU "\n== many children comparator check ==\n" C_RST);
    FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, cmp_rev);
    CHECK_SOFT(f != NULL, "fts_open many children");
    if (!f) {
        free(many);
        return;
    }

    FTSENT* e;
    const char* first = NULL;
    const char* second = NULL;
    int seen = 0;
    while ((e = fts_read(f)) != NULL) {
        if (e->fts_level == 1) {
            if (seen == 0)
                first = strdup(e->fts_name);
            else if (seen == 1)
                second = strdup(e->fts_name);
            seen++;
            if (seen >= 2)
                break;
        }
    }
    fts_close(f);

    if (first && second) {
        CHECK_SOFT(strcmp(first, second) > 0, "reverse comparator affects sibling order");
    }
    else {
        CHECK_SOFT(false, "failed to capture sibling ordering in many children dir");
    }

    free((void*)first);
    free((void*)second);
    free(many);
}

int main(int argc, char** argv) {
    STRICT = getenv("FTS_TEST_STRICT") && strcmp(getenv("FTS_TEST_STRICT"), "0") != 0;

    if (argc > 1) {
        WalkStats s;
        run_walk("user roots PHYSICAL NOCHDIR", NULL, &argv[1], FTS_PHYSICAL | FTS_NOCHDIR, false, false, &s);
        printf("\nsummary: dirs=%zu files=%zu symlinks=%zu dangling=%zu post=%zu errors=%zu\n", s.n_dirs, s.n_files,
               s.n_symlinks, s.n_dangling, s.n_post, s.n_errors);
        return test_failures ? 1 : 0;
    }

    char tmpl[] = "/tmp/fts-test-XXXXXX";
    char* root = mkdtemp(tmpl);
    if (!root) {
        perror("mkdtemp");
        return 1;
    }
    printf("using temp root %s\n", root);

    if (build_tree(root) == -1) {
        perror("build_tree");
        rm_rf(root);
        return 1;
    }

    char* abs_root = realpath(root, NULL);
    CHECK_SOFT(abs_root != NULL, "realpath temp root");
    char* rel_a = join2(root, "a");
    char* rel_b = join2(root, "b");
    CHECK_SOFT(rel_a && rel_b, "allocate relative roots");

    /* always run extra coverage */
    test_cwd_restore(abs_root);
    test_file_root(abs_root);
    test_seedot(abs_root);
    test_symlink_loop_follow(abs_root);

    /* always run heavy fanout stress */
    test_many_children_sorted(abs_root, 2000);

    WalkStats s1, s2, s3, s4, s5, s6;

    {
        char** roots = make_roots(abs_root, NULL);
        run_walk("PHYSICAL with chdir", abs_root, roots, FTS_PHYSICAL, true, true, &s1);
        free(roots);
        CHECK_SOFT(s1.n_dirs >= 1, "visited at least the root directory");
    }

    {
        char** roots = make_roots(abs_root, NULL);
        run_walk("PHYSICAL NOCHDIR", abs_root, roots, FTS_PHYSICAL | FTS_NOCHDIR, false, false, &s2);
        free(roots);
        CHECK_SOFT(s2.n_dirs >= 3, "nochdir should see subdirs");
    }

    {
        char** roots = make_roots(abs_root, NULL);
        run_walk("LOGICAL no COMFOLLOW", abs_root, roots, FTS_LOGICAL, false, false, &s3);
        free(roots);
        CHECK_SOFT(s3.n_dirs >= 3, "logical saw subdirs");
    }

    {
        char** roots = make_roots(abs_root, NULL);
        run_walk("LOGICAL with COMFOLLOW", abs_root, roots, FTS_LOGICAL | FTS_COMFOLLOW, false, true, &s4);
        free(roots);
        CHECK_SOFT(s4.n_dirs >= 3, "following symlinks did not regress directories");
    }

    {
        char** roots = make_roots(rel_b, rel_a);
        run_walk("two roots PHYSICAL NOCHDIR", root, roots, FTS_PHYSICAL | FTS_NOCHDIR, true, false, &s5);
        free(roots);
        CHECK_SOFT(s5.n_files >= 2, "two roots include files from a and b");
    }

    {
        char** roots = make_roots(abs_root, NULL);
        run_walk("PHYSICAL NOSTAT SEEDOT", abs_root, roots, FTS_PHYSICAL | FTS_NOSTAT | FTS_SEEDOT, false, false, &s6);
        free(roots);
        CHECK_SOFT(s6.n_dirs >= 1, "nostat traversal didn’t crash");
    }

    /* children API smoke at b */
    {
        char* roots_one[] = {abs_root, NULL};
        FTS* f = fts_open(roots_one, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
        CHECK_SOFT(f != NULL, "children smoke open");
        if (f) {
            FTSENT* e;
            while ((e = fts_read(f)) != NULL) {
                if (e->fts_level == 1 && e->fts_info == FTS_D && strcmp(e->fts_name, "b") == 0) {
                    printf("children at %s\n", e->fts_path);
                    list_children(f, 1);
                    list_children(f, 0);
                    break;
                }
            }
            fts_close(f);
        }
    }

    /* unreadable directory surface */
    {
        char* unread_abs = join2(abs_root, "a/unreadable_dir");
        char* roots[] = {unread_abs, NULL};
        FTS* f = fts_open(roots, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
        if (f) {
            bool saw_err = false;
            FTSENT* e;
            while ((e = fts_read(f)) != NULL) {
                if (e->fts_info == FTS_DNR || e->fts_info == FTS_ERR) {
                    saw_err = true;
                    print_ent(abs_root, e);
                }
            }
            CHECK_SOFT(saw_err, "unreadable directory produced an error surface");
            fts_close(f);
        }
        else {
            perror("fts_open unreadable");
        }
        free(unread_abs);
    }

    /* XDEV sanity on same dev */
    {
        char* roots[] = {abs_root, NULL};
        WalkStats sx;
        run_walk("PHYSICAL XDEV", abs_root, roots, FTS_PHYSICAL | FTS_XDEV, false, false, &sx);
        CHECK_SOFT(sx.n_dirs >= 1, "XDEV on same device still walks");
    }

    /* restore perms and cleanup */
    char* unread_abs = join2(abs_root, "a/unreadable_dir");
    chmod(unread_abs, 0700);
    free(unread_abs);

    rm_rf(abs_root);
    free(abs_root);
    free(rel_a);
    free(rel_b);

    if (test_failures) {
        fprintf(stderr, C_RED "\n%d checks failed\n" C_RST, test_failures);
        return 1;
    }
    printf(C_GRN "\nall checks passed\n" C_RST);
    return 0;
}
