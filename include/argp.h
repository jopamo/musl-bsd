#ifndef ARGP_H_INCLUDED
#define ARGP_H_INCLUDED

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
typedef ptrdiff_t ssize_t;
#else
#include <unistd.h>
#endif

#ifndef __error_t_defined
typedef int error_t;
#define __error_t_defined
#endif

struct argp_option {
    const char* name;
    int key;
    const char* arg;
    int flags;
    const char* doc;
    int group;
};

#define OPTION_ARG_OPTIONAL 0x01
#define OPTION_HIDDEN 0x02
#define OPTION_ALIAS 0x04
#define OPTION_DOC 0x08
#define OPTION_NO_USAGE 0x10

struct argp;
struct argp_child;

struct argp_state {
    const struct argp* root_argp;
    int argc;
    char** argv;
    int next;
    unsigned flags;
    unsigned arg_num;
    int quoted;
    void* input;
    void** child_inputs;
    void* hook;
    char* name;
    FILE* err_stream;
    FILE* out_stream;
    void* pstate;
};

typedef error_t (*argp_parser_t)(int key, char* arg, struct argp_state* state);

struct argp {
    const struct argp_option* options;
    argp_parser_t parser;
    const char* args_doc;
    const char* doc;
    const struct argp_child* children;
    char* (*help_filter)(int key, const char* text, void* input);
    const char* argp_domain;
};

struct argp_child {
    const struct argp* argp;
    int flags;
    const char* header;
    int group;
};

#define ARGP_KEY_ARG 0
#define ARGP_KEY_ARGS 0x1000006
#define ARGP_KEY_END 0x1000001
#define ARGP_KEY_NO_ARGS 0x1000002
#define ARGP_KEY_INIT 0x1000003
#define ARGP_KEY_FINI 0x1000007
#define ARGP_KEY_SUCCESS 0x1000004
#define ARGP_KEY_ERROR 0x1000005

#define ARGP_KEY_HELP_PRE_DOC 0x2000001
#define ARGP_KEY_HELP_POST_DOC 0x2000002
#define ARGP_KEY_HELP_HEADER 0x2000003
#define ARGP_KEY_HELP_EXTRA 0x2000004
#define ARGP_KEY_HELP_DUP_ARGS_NOTE 0x2000005
#define ARGP_KEY_HELP_ARGS_DOC 0x2000006

#define ARGP_PARSE_ARGV0 0x01
#define ARGP_NO_ERRS 0x02
#define ARGP_NO_ARGS 0x04
#define ARGP_IN_ORDER 0x08
#define ARGP_NO_HELP 0x10
#define ARGP_NO_EXIT 0x20
#define ARGP_LONG_ONLY 0x40
#define ARGP_SILENT (ARGP_NO_EXIT | ARGP_NO_ERRS | ARGP_NO_HELP)

#ifndef ARGP_ERR_UNKNOWN
#define ARGP_ERR_UNKNOWN E2BIG
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern error_t argp_parse(const struct argp* argp, int argc, char** argv, unsigned flags, int* end_index, void* input);

extern error_t __argp_parse(const struct argp* argp,
                            int argc,
                            char** argv,
                            unsigned flags,
                            int* end_index,
                            void* input);

extern const char* argp_program_version;
extern void (*argp_program_version_hook)(FILE*, struct argp_state*);
extern const char* argp_program_bug_address;

extern error_t argp_err_exit_status;

#define ARGP_HELP_USAGE 0x01
#define ARGP_HELP_SHORT_USAGE 0x02
#define ARGP_HELP_SEE 0x04
#define ARGP_HELP_LONG 0x08
#define ARGP_HELP_PRE_DOC 0x10
#define ARGP_HELP_POST_DOC 0x20
#define ARGP_HELP_BUG_ADDR 0x40
#define ARGP_HELP_EXIT_ERR 0x100
#define ARGP_HELP_EXIT_OK 0x200

#define ARGP_HELP_STD_ERR (ARGP_HELP_SEE | ARGP_HELP_EXIT_ERR)
#define ARGP_HELP_STD_USAGE (ARGP_HELP_SHORT_USAGE | ARGP_HELP_SEE | ARGP_HELP_EXIT_ERR)
#define ARGP_HELP_STD_HELP                                                                                 \
    (ARGP_HELP_SHORT_USAGE | ARGP_HELP_LONG | ARGP_HELP_EXIT_OK | ARGP_HELP_PRE_DOC | ARGP_HELP_POST_DOC | \
     ARGP_HELP_BUG_ADDR)

extern void argp_help(const struct argp* argp, FILE* stream, unsigned flags, char* name);
extern void __argp_help(const struct argp* argp, FILE* stream, unsigned flags, char* name);

extern void argp_state_help(const struct argp_state* state, FILE* stream, unsigned flags);
extern void __argp_state_help(const struct argp_state* state, FILE* stream, unsigned flags);

extern void argp_usage(const struct argp_state* state);
extern void __argp_usage(const struct argp_state* state);

extern void argp_error(const struct argp_state* state, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
extern void __argp_error(const struct argp_state* state, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

extern void argp_failure(const struct argp_state* state, int status, int errnum, const char* fmt, ...)
    __attribute__((format(printf, 4, 5)));
extern void __argp_failure(const struct argp_state* state, int status, int errnum, const char* fmt, ...)
    __attribute__((format(printf, 4, 5)));

struct argp_fmtstream {
    FILE* stream;
    size_t lmargin;
    size_t rmargin;
    ssize_t wmargin;

    size_t point_offs;
    ssize_t point_col;

    char* buf;
    char* p;
    char* end;
};
typedef struct argp_fmtstream* argp_fmtstream_t;

extern argp_fmtstream_t __argp_make_fmtstream(FILE* stream, size_t lmargin, size_t rmargin, ssize_t wmargin);
extern argp_fmtstream_t argp_make_fmtstream(FILE* stream, size_t lmargin, size_t rmargin, ssize_t wmargin);
extern void __argp_fmtstream_free(argp_fmtstream_t fs);
extern void argp_fmtstream_free(argp_fmtstream_t fs);

extern ssize_t __argp_fmtstream_printf(argp_fmtstream_t fs, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
extern ssize_t argp_fmtstream_printf(argp_fmtstream_t fs, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

#define argp_fmtstream_lmargin(fs) ((fs)->lmargin)
#define argp_fmtstream_rmargin(fs) ((fs)->rmargin)
#define argp_fmtstream_wmargin(fs) ((fs)->wmargin)

extern void _argp_fmtstream_update(argp_fmtstream_t fs);
extern void __argp_fmtstream_update(argp_fmtstream_t fs);
extern int _argp_fmtstream_ensure(argp_fmtstream_t fs, size_t amount);
extern int __argp_fmtstream_ensure(argp_fmtstream_t fs, size_t amount);

static inline size_t __argp_fmtstream_write(argp_fmtstream_t fs __attribute__((unused)),
                                            const char* str __attribute__((unused)),
                                            size_t len __attribute__((unused))) {
    return 0;
}

static inline int __argp_fmtstream_puts(argp_fmtstream_t fs __attribute__((unused)),
                                        const char* str __attribute__((unused))) {
    return 0;
}

static inline int __argp_fmtstream_putc(argp_fmtstream_t fs __attribute__((unused)), int ch __attribute__((unused))) {
    return 0;
}

static inline size_t __argp_fmtstream_set_lmargin(argp_fmtstream_t fs, size_t lm) {
    size_t old = fs->lmargin;
    fs->lmargin = lm;
    return old;
}

static inline size_t __argp_fmtstream_set_rmargin(argp_fmtstream_t fs, size_t rm) {
    size_t old = fs->rmargin;
    fs->rmargin = rm;
    return old;
}

static inline size_t __argp_fmtstream_set_wmargin(argp_fmtstream_t fs, size_t wm) {
    size_t old = (size_t)fs->wmargin;
    fs->wmargin = (ssize_t)wm;
    return old;
}

static inline size_t __argp_fmtstream_point(argp_fmtstream_t fs) {
    return fs->point_col >= 0 ? (size_t)fs->point_col : 0;
}

static inline int __option_is_end(const struct argp_option* opt) {
    return !opt->key && !opt->name && !opt->doc && !opt->group;
}

static inline int __option_is_short(const struct argp_option* opt) {
    if (opt->flags & OPTION_DOC)
        return 0;

    return (opt->key > 0 && opt->key <= 0xFF && isprint(opt->key));
}

#define _option_is_end __option_is_end
#define _option_is_short __option_is_short

static inline char* __argp_basename(char* name) {
    if (!name)
        return (char*)"";
    char* slash = strrchr(name, '/');
    return slash ? slash + 1 : name;
}

#ifdef __cplusplus
}
#endif

#endif /* ARGP_H_INCLUDED */
