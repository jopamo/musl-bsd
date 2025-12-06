#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#pragma once

#include <fts.h>
#include <stdbool.h>
#include <stddef.h>

struct fts_walk_stats {
    size_t n_dirs;
    size_t n_files;
    size_t n_symlinks;
    size_t n_dangling;
    size_t n_post;
    size_t n_errors;
};

struct fts_test_tree {
    char* root;
    char* abs_root;
    char* rel_a;
    char* rel_b;
};

extern int fts_test_failures;
extern int fts_test_strict;

void fts_set_strict_from_env(void);
void fts_check(int cond, const char* fmt, ...);
void fts_check_soft(int cond, const char* fmt, ...);
int fts_exit_code(void);

char* fts_join2(const char* a, const char* b);
int fts_write_file(const char* p, const char* text);
int fts_build_tree(const char* root);
int fts_build_symlink_loop(const char* abs_root);
int fts_build_order_tree(const char* abs_root);
int fts_build_many(const char* root, const char* sub, int n);
int fts_rm_rf(const char* path);

int fts_test_tree_init(struct fts_test_tree* tree);
void fts_test_tree_cleanup(struct fts_test_tree* tree);

void fts_reset_stats(struct fts_walk_stats* s);
void fts_account(struct fts_walk_stats* s, const FTSENT* e);
const char* fts_info_label(int info);
int fts_cmp_rev(const FTSENT** a, const FTSENT** b);
int fts_cmp_asc(const FTSENT** a, const FTSENT** b);
void fts_list_children(FTS* fts, int nameonly);
void fts_print_ent(const char* root_prefix, const FTSENT* e);

void fts_run_walk(const char* label,
                  const char* root_prefix,
                  char* const* roots,
                  int opts,
                  bool use_cmp,
                  bool try_instr,
                  struct fts_walk_stats* out);

char** fts_make_roots(const char* a, const char* b);
