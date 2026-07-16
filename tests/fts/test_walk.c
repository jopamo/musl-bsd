#include "test_support.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define C_BLU "\033[34m"
#define C_YEL "\033[33m"
#define C_RST "\033[0m"

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

static void fts_reset_stats(struct fts_walk_stats* s) {
    memset(s, 0, sizeof(*s));
}

static void fts_account(struct fts_walk_stats* s, const FTSENT* e) {
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
