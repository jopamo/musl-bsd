#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#ifndef ARGP_H
#define ARGP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int error_t;

struct argp_state;
struct argp;

typedef error_t (*argp_parser_t)(int key, char* arg, struct argp_state* state);

#define OPTION_ALIAS 0x1
#define OPTION_DOC 0x2
#define OPTION_HIDDEN 0x4
#define OPTION_NO_USAGE 0x8
#define OPTION_ARG_OPTIONAL 0x10

#define ARGP_KEY_INIT -10
#define ARGP_KEY_NO_ARGS -11
#define ARGP_KEY_ARG -12
#define ARGP_KEY_ARGS -13
#define ARGP_KEY_END -14
#define ARGP_KEY_ERROR -15
#define ARGP_KEY_SUCCESS -16
#define ARGP_KEY_FINI -17

#define ARGP_KEY_HELP_HEADER -18
#define ARGP_KEY_HELP_PRE_DOC -19
#define ARGP_KEY_HELP_POST_DOC -20
#define ARGP_KEY_HELP_EXTRA -21
#define ARGP_KEY_HELP_ARGS_DOC -22
#define ARGP_KEY_HELP_DUP_ARGS_NOTE -23

struct argp_option {
    const char* name;
    int key;
    const char* arg;
    int flags;
    const char* doc;
    int group;
};

struct argp_child {
    const struct argp* argp;
    int flags;
    const char* header;
    int group;
};

struct argp {
    const struct argp_option* options;
    argp_parser_t parser;
    const char* args_doc;
    const char* doc;
    const struct argp_child* children;

    char* (*help_filter)(int key, const char* text, void* input);

    const char* argp_domain;
};

struct argp_state {
    const struct argp* root_argp;
    int argc;
    char** argv;
    int next;
    unsigned flags;
    unsigned arg_num;
    void* input;
    void** child_inputs;
    void* hook;
    FILE* err_stream;
    FILE* out_stream;
    const char* name;
    int quoted;
    void* pstate;
};

#define ARGP_NO_ERRS 0x1
#define ARGP_NO_EXIT 0x2
#define ARGP_LONG_ONLY 0x4
#define ARGP_IN_ORDER 0x8
#define ARGP_NO_ARGS 0x10
#define ARGP_PARSE_ARGV0 0x20

#define ARGP_HELP_USAGE 0x01
#define ARGP_HELP_SHORT_USAGE 0x02
#define ARGP_HELP_LONG 0x04
#define ARGP_HELP_POST_DOC 0x08
#define ARGP_HELP_PRE_DOC 0x10
#define ARGP_HELP_SEE 0x20
#define ARGP_HELP_BUG_ADDR 0x40
#define ARGP_HELP_EXIT_ERR 0x80
#define ARGP_HELP_EXIT_OK 0x100
#define ARGP_HELP_STD_ERR (ARGP_HELP_SHORT_USAGE | ARGP_HELP_EXIT_ERR)
#define ARGP_HELP_STD_HELP (ARGP_HELP_LONG | ARGP_HELP_USAGE | ARGP_HELP_EXIT_OK)

extern const char* argp_program_bug_address;
extern const char* argp_program_version;
extern void (*argp_program_version_hook)(FILE* stream, struct argp_state* state);

error_t argp_parse(const struct argp* argp, int argc, char** argv, unsigned flags, int* arg_index, void* input);
void argp_help(const struct argp* argp, FILE* stream, unsigned flags, char* name);
void argp_state_help(const struct argp_state* state, FILE* stream, unsigned flags);

void argp_usage(const struct argp_state* state);
void argp_error(const struct argp_state* state, const char* fmt, ...);
void argp_failure(const struct argp_state* state, int status, int errnum, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif

const char* argp_program_bug_address = NULL;
const char* argp_program_version = NULL;
void (*argp_program_version_hook)(FILE*, struct argp_state*) = NULL;

static error_t argp_err_exit_status = 64;

static char* argp_basename(char* path) {
    char* slash = strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

static char* argp_short_program_name(const struct argp_state* state) {
    if (state && state->name)
        return (char*)state->name;
    return (char*)"program";
}

#ifndef dgettext
#define dgettext(domain, msgid) (msgid)
#endif

static void __argp_verror(const struct argp_state* state, const char* fmt, va_list ap) {
    if (!state || !(state->flags & ARGP_NO_ERRS)) {
        FILE* stream = state ? state->err_stream : stderr;
        if (stream) {
            fprintf(stream, "%s: ", state && state->name ? state->name : "argp");
            vfprintf(stream, fmt, ap);
            fputc('\n', stream);
        }
    }
}

void argp_error(const struct argp_state* state, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    __argp_verror(state, fmt, ap);
    va_end(ap);

    if (!state || !(state->flags & ARGP_NO_ERRS)) {
        argp_state_help(state, state ? state->err_stream : stderr, ARGP_HELP_STD_ERR);
    }
}

void argp_failure(const struct argp_state* state, int status, int errnum, const char* fmt, ...) {
    if (!state || !(state->flags & ARGP_NO_ERRS)) {
        FILE* stream = state ? state->err_stream : stderr;
        if (stream) {
            va_list ap;
            va_start(ap, fmt);
            fprintf(stream, "%s", state && state->name ? state->name : "argp");
            if (fmt && *fmt) {
                fputs(": ", stream);
                vfprintf(stream, fmt, ap);
            }
            va_end(ap);
            if (errnum)
                fprintf(stream, ": %s", strerror(errnum));
            fputc('\n', stream);

            if (status && (!state || !(state->flags & ARGP_NO_EXIT)))
                exit(status);
        }
    }
}

void argp_usage(const struct argp_state* state) {
    argp_state_help(state, state ? state->err_stream : stderr, ARGP_HELP_STD_ERR);
}

static int argp_option_is_end(const struct argp_option* o) {
    return (!o->key && !o->name && !o->doc && !o->group);
}
static int argp_option_is_short(const struct argp_option* o) {
    if (o->flags & OPTION_DOC)
        return 0;
    if (o->key <= 0 || o->key > 255)
        return 0;
    return isprint(o->key);
}
static int argp_option_is_visible(const struct argp_option* o) {
    return !(o->flags & OPTION_HIDDEN);
}
static int argp_option_is_alias(const struct argp_option* o) {
    return (o->flags & OPTION_ALIAS);
}

static void argp_help_print_options(const struct argp* argp, const struct argp_state* state, FILE* stream) {
    if (!argp)
        return;

    if (argp->options) {
        for (const struct argp_option* o = argp->options; !argp_option_is_end(o); o++) {
            if (!argp_option_is_visible(o) || argp_option_is_alias(o))
                continue;

            int shortable = argp_option_is_short(o);

            if (shortable) {
                fprintf(stream, "  -%c", o->key);
                if (o->arg) {
                    if (o->flags & OPTION_ARG_OPTIONAL)
                        fprintf(stream, "[=%s]", dgettext(argp->argp_domain, o->arg));
                    else
                        fprintf(stream, " %s", dgettext(argp->argp_domain, o->arg));
                }
            }

            if (o->name) {
                if (shortable)
                    fputs(", ", stream);
                else
                    fputs("  ", stream);

                fprintf(stream, "--%s", o->name);
                if (o->arg) {
                    if (o->flags & OPTION_ARG_OPTIONAL)
                        fprintf(stream, "[=%s]", dgettext(argp->argp_domain, o->arg));
                    else
                        fprintf(stream, "=%s", dgettext(argp->argp_domain, o->arg));
                }
            }

            if (o->doc && *o->doc) {
                fprintf(stream, "\n      %s", dgettext(argp->argp_domain, o->doc));
            }
            fputc('\n', stream);
        }
    }

    if (argp->children) {
        for (const struct argp_child* ch = argp->children; ch->argp; ch++) {
            if (ch->header && *ch->header)
                fprintf(stream, "\n%s\n", dgettext(ch->argp->argp_domain, ch->header));
            argp_help_print_options(ch->argp, state, stream);
        }
    }
}

static void argp_help_print_doc(const struct argp* argp, const struct argp_state* state, FILE* stream, int post) {
    if (!argp || !argp->doc)
        return;

    const char* full = dgettext(argp->argp_domain, argp->doc);
    const char* sep = strchr(full, '\v');
    const char* use = NULL;
    if (!sep) {
        if (!post)
            use = full;
        else
            return;
    }
    else {
        if (!post) {
            static char* tmp;
            free(tmp);
            size_t len = (size_t)(sep - full);
            tmp = (char*)malloc(len + 1);
            if (!tmp)
                return;
            memcpy(tmp, full, len);
            tmp[len] = 0;
            use = tmp;
        }
        else {
            use = sep + 1;
        }
    }
    if (!use || !*use)
        return;

    if (argp->help_filter) {
        char* f = argp->help_filter(post ? ARGP_KEY_HELP_POST_DOC : ARGP_KEY_HELP_PRE_DOC, use,
                                    (state ? state->input : NULL));
        if (f) {
            fprintf(stream, "%s\n", f);
            if (f != use)
                free(f);
        }
    }
    else {
        fprintf(stream, "%s\n", use);
    }
}

static void _argp_help(const struct argp* argp,
                       const struct argp_state* state,
                       FILE* stream,
                       unsigned flags,
                       char* name) {
    if (!stream)
        return;
    if (!name && state)
        name = argp_short_program_name(state);
    if (!name)
        name = (char*)"program";

    if (flags & (ARGP_HELP_USAGE | ARGP_HELP_SHORT_USAGE)) {
        fprintf(stream, "%s %s %s", dgettext(argp ? argp->argp_domain : NULL, "Usage:"), name,
                dgettext(argp ? argp->argp_domain : NULL, "[OPTION...]"));
        if (argp && argp->args_doc && *argp->args_doc) {
            fprintf(stream, " %s", dgettext(argp->argp_domain, argp->args_doc));
        }
        fputc('\n', stream);
    }

    if (flags & ARGP_HELP_PRE_DOC) {
        argp_help_print_doc(argp, state, stream, 0);
    }

    if (flags & ARGP_HELP_LONG) {
        fputc('\n', stream);
        argp_help_print_options(argp, state, stream);
    }

    if (flags & ARGP_HELP_POST_DOC) {
        fputc('\n', stream);
        argp_help_print_doc(argp, state, stream, 1);
    }

    if (flags & ARGP_HELP_SEE) {
        fprintf(stream, "\nTry `%s --help' or `%s --usage' for more information.\n", name, name);
    }

    if ((flags & ARGP_HELP_BUG_ADDR) && argp_program_bug_address) {
        fprintf(stream, "Report bugs to %s.\n", argp_program_bug_address);
    }
}

void argp_help(const struct argp* argp, FILE* stream, unsigned flags, char* name) {
    _argp_help(argp, NULL, stream, flags, name);
}

void argp_state_help(const struct argp_state* state, FILE* stream, unsigned flags) {
    if (!state || !(state->flags & ARGP_NO_ERRS)) {
        _argp_help(state ? state->root_argp : NULL, state, stream, flags,
                   (state && state->name) ? (char*)state->name : NULL);

        if (!state || !(state->flags & ARGP_NO_EXIT)) {
            if (flags & ARGP_HELP_EXIT_ERR)
                exit(argp_err_exit_status);
            if (flags & ARGP_HELP_EXIT_OK)
                exit(0);
        }
    }
}

#define ARGP_EBADKEY 9999

struct group {
    argp_parser_t parser;
    const struct argp* argp;
    unsigned args_processed;
    struct group* parent;
    unsigned parent_index;
    void* input;
    void** child_inputs;
    void* hook;
};

struct parser {
    const struct argp* argp;
    const char* posixly_correct;
    int args_only;

    enum { REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER } ordering;
    int first_nonopt;
    int last_nonopt;

    char* short_opts;
    char* nextchar;

    struct group* groups;
    struct group* egroup;
    void** child_inputs;

    struct argp_state state;
    void* storage;
};

static error_t group_parse(struct group* grp, struct argp_state* st, int key, char* arg);

static int option_is_end(const struct argp_option* opt) {
    return (!opt->key && !opt->name && !opt->doc && !opt->group);
}
static int option_is_short(const struct argp_option* opt) {
    if (opt->flags & OPTION_DOC)
        return 0;
    if (opt->key > 0 && opt->key <= 255 && isprint((unsigned char)opt->key))
        return 1;
    return 0;
}

struct parser_sizes {
    size_t short_len;
    size_t num_groups;
    size_t num_child_inputs;
};

static void calc_sizes(const struct argp* argp, struct parser_sizes* sz) {
    if (!argp)
        return;
    const struct argp_option* o = argp->options;
    if (o || argp->parser) {
        sz->num_groups++;
        if (o) {
            for (; !option_is_end(o); o++) {
                if (option_is_short(o)) {
                    sz->short_len++;
                }
            }
        }
    }
    if (argp->children) {
        for (const struct argp_child* c = argp->children; c->argp; c++) {
            calc_sizes(c->argp, sz);
            sz->num_child_inputs++;
        }
    }
}

static struct group* convert_options(const struct argp* argp,
                                     struct group* parent,
                                     unsigned parent_index,
                                     struct group* grp,
                                     void*** child_inputs_end,
                                     char** short_end);

static void parser_convert(struct parser* p, const struct argp* argp) {
    p->argp = argp;
    if (!argp) {
        p->egroup = p->groups;
        return;
    }
    void** child_end = p->child_inputs;
    char* s_end = p->short_opts;
    p->egroup = convert_options(argp, NULL, 0, p->groups, &child_end, &s_end);
    if (p->short_opts)
        *s_end = '\0';
}

static struct group* convert_options(const struct argp* argp,
                                     struct group* parent,
                                     unsigned parent_index,
                                     struct group* grp,
                                     void*** child_inputs_end,
                                     char** short_end) {
    if (argp && (argp->options || argp->parser)) {
        grp->parser = argp->parser;
        grp->argp = argp;
        grp->args_processed = 0;
        grp->parent = parent;
        grp->parent_index = parent_index;
        grp->input = NULL;
        grp->child_inputs = NULL;
        grp->hook = NULL;

        if (*short_end && argp->options) {
            for (const struct argp_option* o = argp->options; !option_is_end(o); o++) {
                if (option_is_short(o)) {
                    **short_end = (char)o->key;
                    (*short_end)++;
                }
            }
        }

        if (argp->children) {
            unsigned kids = 0;
            for (const struct argp_child* c = argp->children; c->argp; c++)
                kids++;
            grp->child_inputs = *child_inputs_end;
            *child_inputs_end += kids;
        }

        parent = grp++;
    }
    else {
        parent = NULL;
    }

    if (argp && argp->children) {
        unsigned idx = 0;
        for (const struct argp_child* c = argp->children; c->argp; c++, idx++) {
            grp = convert_options(c->argp, parent, idx, grp, child_inputs_end, short_end);
        }
    }
    return grp;
}

static error_t parser_init(struct parser* p,
                           const struct argp* argp,
                           int argc,
                           char** argv,
                           unsigned flags,
                           void* input) {
    memset(p, 0, sizeof(*p));
    p->posixly_correct = getenv("POSIXLY_CORRECT");

    if (flags & ARGP_IN_ORDER)
        p->ordering = RETURN_IN_ORDER;
    else if (flags & ARGP_NO_ARGS)
        p->ordering = REQUIRE_ORDER;
    else if (p->posixly_correct)
        p->ordering = REQUIRE_ORDER;
    else
        p->ordering = PERMUTE;

    struct parser_sizes sz = {0, 0, 0};
    calc_sizes(argp, &sz);

    if (!(flags & ARGP_LONG_ONLY)) {
        sz.short_len = 0;
    }

    size_t glen = (sz.num_groups + 1) * sizeof(struct group);
    size_t clen = sz.num_child_inputs * sizeof(void*);
    size_t slen = sz.short_len + 1;

    p->storage = malloc(glen + clen + slen);
    if (!p->storage)
        return ENOMEM;

    p->groups = (struct group*)p->storage;
    p->child_inputs = (void**)((char*)p->storage + glen);
    memset(p->child_inputs, 0, clen);
    if (flags & ARGP_LONG_ONLY)
        p->short_opts = (char*)((char*)p->storage + glen + clen);
    else
        p->short_opts = NULL;

    parser_convert(p, argp);

    memset(&p->state, 0, sizeof(p->state));
    p->state.root_argp = p->argp;
    p->state.argc = argc;
    p->state.argv = argv;
    p->state.flags = flags;
    p->state.err_stream = stderr;
    p->state.out_stream = stdout;
    p->state.pstate = p;

    if (p->groups < p->egroup)
        p->groups->input = input;

    for (struct group* g = p->groups; g < p->egroup; g++) {
        if (g->parser) {
            error_t e = group_parse(g, &p->state, ARGP_KEY_INIT, NULL);
            if (e != ARGP_EBADKEY && e)
                return e;
        }
    }

    if (argv[0] && !(flags & ARGP_PARSE_ARGV0)) {
        p->state.name = argp_basename(argv[0]);
        p->state.next = 1;
    }
    else {
        p->state.name = "program";
    }
    return 0;
}

static error_t parser_finalize(struct parser* p, error_t err, int arg_ebadkey, int* end_index) {
    if (err == ARGP_EBADKEY && arg_ebadkey) {
        err = 0;
    }
    if (!err) {
        if (p->state.next == p->state.argc) {
            for (struct group* g = p->groups; g < p->egroup; g++) {
                if (!g->args_processed) {
                    error_t e = group_parse(g, &p->state, ARGP_KEY_NO_ARGS, NULL);
                    if (e != ARGP_EBADKEY && e)
                        err = e;
                }
            }
            for (struct group* g = p->egroup; g-- > p->groups;) {
                error_t e = group_parse(g, &p->state, ARGP_KEY_END, NULL);
                if (e != ARGP_EBADKEY && e)
                    err = e;
            }
            if (end_index)
                *end_index = p->state.next;
        }
        else if (end_index) {
            *end_index = p->state.next;
        }
        else {
            if (!(p->state.flags & ARGP_NO_ERRS) && p->state.err_stream) {
                fprintf(p->state.err_stream, "%s: Too many arguments\n", p->state.name);
            }
            err = ARGP_EBADKEY;
        }
    }
    if (err) {
        if (err == ARGP_EBADKEY) {
            argp_state_help(&p->state, p->state.err_stream, ARGP_HELP_STD_ERR);
        }

        for (struct group* g = p->groups; g < p->egroup; g++) {
            group_parse(g, &p->state, ARGP_KEY_ERROR, NULL);
        }
    }
    else {
        for (struct group* g = p->egroup; g-- > p->groups;) {
            error_t e = group_parse(g, &p->state, ARGP_KEY_SUCCESS, NULL);
            if (e != ARGP_EBADKEY && e)
                err = e;
        }
    }

    for (struct group* g = p->egroup; g-- > p->groups;) {
        group_parse(g, &p->state, ARGP_KEY_FINI, NULL);
    }
    if (err == ARGP_EBADKEY)
        err = EINVAL;

    free(p->storage);
    return err;
}

static error_t group_parse(struct group* grp, struct argp_state* st, int key, char* arg) {
    if (!grp->parser)
        return ARGP_EBADKEY;
    st->hook = grp->hook;
    st->input = grp->input;
    st->child_inputs = grp->child_inputs;
    st->arg_num = grp->args_processed;
    error_t e = grp->parser(key, arg, st);
    grp->hook = st->hook;
    return e;
}

static error_t parser_parse_arg(struct parser* p, char* val) {
    int index = p->state.next;
    error_t e = ARGP_EBADKEY;
    for (struct group* g = p->groups; g < p->egroup && e == ARGP_EBADKEY; g++) {
        p->state.next++;
        e = group_parse(g, &p->state, ARGP_KEY_ARG, val);
        if (e == ARGP_EBADKEY) {
            p->state.next--;
            e = group_parse(g, &p->state, ARGP_KEY_ARGS, NULL);
        }
        if (!e) {
            g->args_processed += (p->state.next - index);
        }
    }
    return e;
}

static void exchange(struct parser* p) {
    int bottom = p->first_nonopt;
    int middle = p->last_nonopt;
    int top = p->state.next;
    char** argv = p->state.argv;
    while (top > middle && middle > bottom) {
        if (top - middle > middle - bottom) {
            int len = middle - bottom;
            for (int i = 0; i < len; i++) {
                char* tmp = argv[bottom + i];
                argv[bottom + i] = argv[top - (middle - bottom) + i];
                argv[top - (middle - bottom) + i] = tmp;
            }
            top -= len;
        }
        else {
            int len = top - middle;
            for (int i = 0; i < len; i++) {
                char* tmp = argv[bottom + i];
                argv[bottom + i] = argv[middle + i];
                argv[middle + i] = tmp;
            }
            bottom += len;
        }
    }
    p->first_nonopt += (p->state.next - p->last_nonopt);
    p->last_nonopt = p->state.next;
}

enum arg_type { ARG_ARG, ARG_SHORT_OPTION, ARG_LONG_OPTION, ARG_LONG_ONLY_OPTION, ARG_QUOTE };
static enum arg_type classify_arg(struct parser* p, char* arg, char** out) {
    if (arg[0] == '-') {
        if (arg[1] == '\0')
            return ARG_ARG;
        else if (arg[1] == '-') {
            if (!arg[2])
                return ARG_QUOTE;
            if (out)
                *out = &arg[2];
            return ARG_LONG_OPTION;
        }
        else {
            if (out)
                *out = &arg[1];
            if (p->state.flags & ARGP_LONG_ONLY) {
                if (arg[2] || !p->short_opts || !strchr(p->short_opts, arg[1]))
                    return ARG_LONG_ONLY_OPTION;
            }
            return ARG_SHORT_OPTION;
        }
    }
    return ARG_ARG;
}

static error_t parser_parse_next(struct parser* p, int* arg_ebadkey) {
    if (p->last_nonopt > p->state.next)
        p->last_nonopt = p->state.next;
    if (p->first_nonopt > p->state.next)
        p->first_nonopt = p->state.next;

    if (p->nextchar) {
        char c = *p->nextchar++;
        struct group* grp = NULL;

        const struct argp_option* opt = NULL;
        for (struct group* g = p->groups; g < p->egroup && !opt; g++) {
            const struct argp_option* o = g->argp->options;
            for (; o && !option_is_end(o); o++) {
                if (o->key == c) {
                    opt = o;
                    grp = g;
                    break;
                }
            }
        }
        if (!opt) {
            fprintf(p->state.err_stream, "%s: invalid short option -- %c\n", p->state.name, c);
            *arg_ebadkey = 0;
            return ARGP_EBADKEY;
        }
        char* value = NULL;
        if (opt->arg) {
            value = p->nextchar;
            p->nextchar = NULL;
            if (!value && !(opt->flags & OPTION_ARG_OPTIONAL)) {
                if (p->state.next >= p->state.argc) {
                    fprintf(p->state.err_stream, "%s: option requires an argument -- %c\n", p->state.name, c);
                    *arg_ebadkey = 0;
                    return ARGP_EBADKEY;
                }
                value = p->state.argv[p->state.next++];
            }
        }
        return group_parse(grp, &p->state, opt->key, value);
    }
    else {
        if (p->args_only) {
            *arg_ebadkey = 1;
            if (p->state.next >= p->state.argc)
                return ARGP_EBADKEY;
            return parser_parse_arg(p, p->state.argv[p->state.next]);
        }
        if (p->state.next >= p->state.argc) {
            *arg_ebadkey = 1;

            if (p->first_nonopt != p->last_nonopt) {
                exchange(p);
                p->state.next = p->first_nonopt;
                p->first_nonopt = p->last_nonopt = 0;
                p->args_only = 1;
                return 0;
            }
            return ARGP_EBADKEY;
        }

        char* arg = p->state.argv[p->state.next];
        char* start = NULL;
        enum arg_type t = classify_arg(p, arg, &start);
        switch (t) {
            case ARG_ARG:
                switch (p->ordering) {
                    case PERMUTE:
                        if (p->first_nonopt == p->last_nonopt)
                            p->first_nonopt = p->last_nonopt = p->state.next;
                        else if (p->last_nonopt != p->state.next)
                            exchange(p);
                        p->state.next++;
                        p->last_nonopt = p->state.next;
                        return 0;
                    case REQUIRE_ORDER:
                        p->args_only = 1;
                        return 0;
                    case RETURN_IN_ORDER:
                        *arg_ebadkey = 1;
                        return parser_parse_arg(p, arg);
                }
                break;
            case ARG_QUOTE:

                p->state.next++;
                if (p->first_nonopt != p->last_nonopt)
                    exchange(p);
                p->args_only = 1;
                return 0;
            case ARG_LONG_ONLY_OPTION:
            case ARG_LONG_OPTION: {
                p->state.next++;
                char* eq = strchr(start, '=');
                if (eq) {
                    *eq++ = '\0';
                }

                const struct argp_option* opt = NULL;
                struct group* grp = NULL;
                for (struct group* g = p->groups; g < p->egroup && !opt; g++) {
                    const struct argp_option* o = g->argp->options;
                    for (; o && !option_is_end(o); o++) {
                        if (o->name && !strcmp(o->name, start)) {
                            opt = o;
                            grp = g;
                            break;
                        }
                    }
                }
                if (!opt) {
                    fprintf(p->state.err_stream, "%s: unrecognized option `--%s'\n", p->state.name, start);
                    *arg_ebadkey = 0;
                    return ARGP_EBADKEY;
                }
                if (eq && !opt->arg) {
                    fprintf(p->state.err_stream, "%s: option `--%s' doesn't allow an argument\n", p->state.name,
                            opt->name);
                    *arg_ebadkey = 0;
                    return ARGP_EBADKEY;
                }
                if (opt->arg && !eq && !(opt->flags & OPTION_ARG_OPTIONAL)) {
                    if (p->state.next >= p->state.argc) {
                        fprintf(p->state.err_stream, "%s: option `--%s' requires an argument\n", p->state.name,
                                opt->name);
                        *arg_ebadkey = 0;
                        return ARGP_EBADKEY;
                    }
                    eq = p->state.argv[p->state.next++];
                }
                *arg_ebadkey = 0;
                return group_parse(grp, &p->state, opt->key, eq);
            }
            case ARG_SHORT_OPTION:
                p->state.next++;
                p->nextchar = start;
                return 0;
        }
    }
    return 0;
}

static error_t default_help_parser(int key, char* arg, struct argp_state* state) {
    (void)arg;
    switch (key) {
        case '?':
            argp_state_help(state, state->out_stream, ARGP_HELP_STD_HELP);
            break;
        case ARGP_KEY_END:
        case ARGP_KEY_INIT:
        case ARGP_KEY_SUCCESS:
        case ARGP_KEY_FINI:
        case ARGP_KEY_NO_ARGS:
        case ARGP_KEY_ARG:
        case ARGP_KEY_ARGS:
            return 0;
        default:
            return ARGP_EBADKEY;
    }
    return 0;
}

static error_t default_version_parser(int key, char* arg, struct argp_state* state) {
    (void)arg;
    switch (key) {
        case 'V':
            if (argp_program_version_hook)
                (*argp_program_version_hook)(state->out_stream, state);
            else if (argp_program_version)
                fprintf(state->out_stream, "%s\n", argp_program_version);
            else
                argp_error(state, "(PROGRAM ERROR) No version known!?");
            if (!(state->flags & ARGP_NO_EXIT))
                exit(0);
            break;
        case ARGP_KEY_END:
        case ARGP_KEY_INIT:
        case ARGP_KEY_SUCCESS:
        case ARGP_KEY_FINI:
        case ARGP_KEY_NO_ARGS:
        case ARGP_KEY_ARG:
        case ARGP_KEY_ARGS:
            return 0;
        default:
            return ARGP_EBADKEY;
    }
    return 0;
}

static const struct argp_option default_version_opts[] = {{"version", 'V', 0, 0, "Display version info", -1}, {0}};

static const struct argp_option default_help_opts[] = {{"help", '?', 0, 0, "Display this help", -1},
                                                       {"usage", 'u', 0, 0, "Display a short usage message", 0},
                                                       {0}};
static struct argp default_help_argp = {default_help_opts, default_help_parser, NULL, NULL, NULL, NULL, NULL};

static struct argp default_version_argp = {default_version_opts, default_version_parser, NULL, NULL, NULL, NULL, NULL};

error_t argp_parse(const struct argp* in_argp, int argc, char** argv, unsigned flags, int* end_index, void* input) {
    const struct argp* argp = in_argp;
    if (!(flags & ARGP_NO_ERRS)) {
        static struct argp_child kids[3];
        static struct argp top;
        memset(kids, 0, sizeof(kids));
        memset(&top, 0, sizeof(top));

        int i = 0;
        if (in_argp)
            kids[i++].argp = in_argp;
        kids[i++].argp = &default_help_argp;

        if (argp_program_version || argp_program_version_hook)
            kids[i++].argp = &default_version_argp;  // Use default_version_argp here

        top.children = kids;
        argp = &top;
    }

    struct parser p;
    error_t err = parser_init(&p, argp, argc, argv, flags, input);
    if (!err) {
        int arg_ebadkey = 0;
        for (;;) {
            error_t e = parser_parse_next(&p, &arg_ebadkey);
            if (e) {
                err = e;
                break;
            }
        }
        err = parser_finalize(&p, err, arg_ebadkey, end_index);
    }
    return err;
}

void* argp_input(const struct argp* child, const struct argp_state* st) {
    if (!st || !child)
        return NULL;
    struct parser* p = (struct parser*)st->pstate;
    if (!p)
        return NULL;
    for (struct group* g = p->groups; g < p->egroup; g++) {
        if (g->argp == child)
            return g->input;
    }
    return NULL;
}
