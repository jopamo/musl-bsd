#include <argp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct io_ctx {
    FILE* stream;
    int filter_calls;
    int filter_input_matches;
    int saw_pre_doc;
    int saw_post_doc;
    int usage_called;
};

static const struct argp_option empty_options[] = {{0}};

static FILE* open_capture(char** out_buf, size_t* out_len) {
    *out_buf = NULL;
    *out_len = 0;
    FILE* stream = open_memstream(out_buf, out_len);
    assert(stream != NULL);
    return stream;
}

static char* slurp_stream(FILE* stream) {
    assert(fseek(stream, 0, SEEK_END) == 0);
    long sz = ftell(stream);
    assert(sz >= 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);

    char* buf = malloc((size_t)sz + 1);
    assert(buf != NULL);
    size_t n = fread(buf, 1, (size_t)sz, stream);
    assert(n == (size_t)sz);
    buf[n] = '\0';
    return buf;
}

static error_t help_parse(int key, char* arg, struct argp_state* state) {
    (void)arg;
    struct io_ctx* ctx = state->input;
    switch (key) {
        case ARGP_KEY_INIT:
            state->out_stream = ctx->stream;
            state->err_stream = ctx->stream;
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
}

static char* help_filter(int key, const char* text, void* input) {
    struct io_ctx* ctx = input;
    if (ctx) {
        ctx->filter_calls++;
        if (input == ctx)
            ctx->filter_input_matches++;
        if (key == ARGP_KEY_HELP_PRE_DOC)
            ctx->saw_pre_doc++;
        if (key == ARGP_KEY_HELP_POST_DOC)
            ctx->saw_post_doc++;
    }
    return (char*)text;
}

static void test_argp_input_used_by_help_filter(void) {
    static const struct argp argp = {empty_options, help_parse,  NULL, "pre help text\vpost help text",
                                     NULL,          help_filter, NULL};

    char* out = NULL;
    size_t out_len = 0;
    struct io_ctx ctx = {.stream = open_capture(&out, &out_len)};

    char prog[] = "prog";
    char help[] = "--help";
    char* argv[] = {prog, help, NULL};

    error_t err = argp_parse(&argp, 2, argv, ARGP_NO_EXIT, NULL, &ctx);
    assert(err == 0);
    assert(fclose(ctx.stream) == 0);

    assert(ctx.filter_calls > 0);
    assert(ctx.filter_input_matches > 0);
    assert(ctx.saw_pre_doc > 0);
    assert(ctx.saw_post_doc > 0);
    assert(strstr(out, "Usage: prog") != NULL);

    free(out);
}

static error_t usage_parse(int key, char* arg, struct argp_state* state) {
    (void)arg;
    struct io_ctx* ctx = state->input;
    switch (key) {
        case ARGP_KEY_INIT:
            state->out_stream = ctx->stream;
            state->err_stream = ctx->stream;
            return 0;
        case 'u':
            ctx->usage_called++;
            argp_usage(state);
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
}

static void test_argp_usage_contract_without_exit(void) {
    static const struct argp_option options[] = {{"trigger-usage", 'u', 0, 0, "emit usage", 0}, {0}};
    static const struct argp argp = {options, usage_parse, NULL, NULL, NULL, NULL, NULL};

    char* out = NULL;
    size_t out_len = 0;
    struct io_ctx ctx = {.stream = open_capture(&out, &out_len)};

    char prog[] = "prog";
    char opt[] = "--trigger-usage";
    char* argv[] = {prog, opt, NULL};

    FILE* stderr_capture = tmpfile();
    assert(stderr_capture != NULL);
    fflush(stderr);
    int saved_stderr = dup(fileno(stderr));
    assert(saved_stderr >= 0);
    assert(dup2(fileno(stderr_capture), fileno(stderr)) >= 0);

    error_t err = argp_parse(&argp, 2, argv, ARGP_NO_EXIT, NULL, &ctx);

    fflush(stderr);
    assert(dup2(saved_stderr, fileno(stderr)) >= 0);
    close(saved_stderr);

    assert(err == 0);
    assert(ctx.usage_called == 1);
    assert(fclose(ctx.stream) == 0);
    free(out);

    char* stderr_text = slurp_stream(stderr_capture);
    assert(strstr(stderr_text, "Usage: prog") != NULL);
    assert(strstr(stderr_text, "Try `prog --help' or `prog --usage' for more information.") != NULL);

    free(stderr_text);
    assert(fclose(stderr_capture) == 0);
}

static error_t accept_need_only(int key, char* arg, struct argp_state* state) {
    (void)state;
    if (key == 'n')
        return arg ? 0 : EINVAL;
    return ARGP_ERR_UNKNOWN;
}

static error_t reject_all_parser(int key, char* arg, struct argp_state* state) {
    (void)key;
    (void)arg;
    (void)state;
    return ARGP_ERR_UNKNOWN;
}

static void test_negative_parse_cases(void) {
    /* Unknown option */
    {
        static const struct argp argp = {empty_options, reject_all_parser, NULL, NULL, NULL, NULL, NULL};
        char prog[] = "prog";
        char bad[] = "--does-not-exist";
        char* argv[] = {prog, bad, NULL};
        error_t err = argp_parse(&argp, 2, argv, ARGP_NO_EXIT | ARGP_NO_ERRS, NULL, NULL);
        assert(err == EINVAL);
    }

    /* Missing required option argument */
    {
        static const struct argp_option options[] = {{"need", 'n', "VAL", 0, "needs an argument", 0}, {0}};
        static const struct argp argp = {options, accept_need_only, NULL, NULL, NULL, NULL, NULL};
        char prog[] = "prog";
        char need[] = "--need";
        char* argv[] = {prog, need, NULL};
        error_t err = argp_parse(&argp, 2, argv, ARGP_NO_EXIT | ARGP_NO_ERRS, NULL, NULL);
        assert(err == EINVAL);
    }

    /* Unexpected positional arg with no end_index return path */
    {
        static const struct argp argp = {empty_options, reject_all_parser, NULL, NULL, NULL, NULL, NULL};
        char prog[] = "prog";
        char positional[] = "payload";
        char* argv[] = {prog, positional, NULL};
        error_t err = argp_parse(&argp, 2, argv, ARGP_NO_EXIT | ARGP_NO_ERRS, NULL, NULL);
        assert(err == EINVAL);
    }
}

struct order_ctx {
    int flag_seen;
    int arg_count;
    const char* args[8];
};

static error_t order_parse(int key, char* arg, struct argp_state* state) {
    struct order_ctx* ctx = state->input;
    switch (key) {
        case 'f':
            ctx->flag_seen++;
            return 0;
        case ARGP_KEY_ARG:
            assert(ctx->arg_count < (int)(sizeof(ctx->args) / sizeof(ctx->args[0])));
            ctx->args[ctx->arg_count++] = arg;
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
}

static void test_argument_ordering_and_permutation_modes(void) {
    static const struct argp_option options[] = {{"flag", 'f', 0, 0, "ordering flag", 0}, {0}};
    static const struct argp argp = {options, order_parse, NULL, NULL, NULL, NULL, NULL};

    const char* old_posix = getenv("POSIXLY_CORRECT");
    char* old_posix_copy = old_posix ? strdup(old_posix) : NULL;

    /* Default getopt permutation mode: option after non-option is still parsed. */
    unsetenv("POSIXLY_CORRECT");
    {
        struct order_ctx ctx = {0};
        char prog[] = "prog";
        char arg1[] = "arg1";
        char flag[] = "--flag";
        char arg2[] = "arg2";
        char* argv[] = {prog, arg1, flag, arg2, NULL};
        error_t err = argp_parse(&argp, 4, argv, ARGP_NO_EXIT, NULL, &ctx);
        assert(err == 0);
        assert(ctx.flag_seen == 1);
        assert(ctx.arg_count == 2);
        assert(strcmp(ctx.args[0], "arg1") == 0);
        assert(strcmp(ctx.args[1], "arg2") == 0);
    }

    /* POSIX mode: option parsing stops at first non-option. */
    setenv("POSIXLY_CORRECT", "1", 1);
    {
        struct order_ctx ctx = {0};
        char prog[] = "prog";
        char arg1[] = "arg1";
        char flag[] = "--flag";
        char arg2[] = "arg2";
        char* argv[] = {prog, arg1, flag, arg2, NULL};
        error_t err = argp_parse(&argp, 4, argv, ARGP_NO_EXIT, NULL, &ctx);
        assert(err == 0);
        assert(ctx.flag_seen == 0);
        assert(ctx.arg_count == 3);
        assert(strcmp(ctx.args[0], "arg1") == 0);
        assert(strcmp(ctx.args[1], "--flag") == 0);
        assert(strcmp(ctx.args[2], "arg2") == 0);
    }

    /* '--' terminates option parsing regardless of mode. */
    {
        struct order_ctx ctx = {0};
        char prog[] = "prog";
        char flag[] = "--flag";
        char stop[] = "--";
        char after[] = "--after";
        char* argv[] = {prog, flag, stop, after, NULL};
        error_t err = argp_parse(&argp, 4, argv, ARGP_NO_EXIT, NULL, &ctx);
        assert(err == 0);
        assert(ctx.flag_seen == 1);
        assert(ctx.arg_count == 1);
        assert(strcmp(ctx.args[0], "--after") == 0);
    }

    if (old_posix_copy) {
        setenv("POSIXLY_CORRECT", old_posix_copy, 1);
        free(old_posix_copy);
    }
    else
        unsetenv("POSIXLY_CORRECT");
}

struct propagation_ctx {
    int parent_error_calls;
    int child_error_calls;
    int parent_end_calls;
    int child_end_calls;
};

static error_t propagation_child_parse(int key, char* arg, struct argp_state* state) {
    struct propagation_ctx* ctx = state->input;
    (void)arg;
    switch (key) {
        case 'c':
            return ECANCELED;
        case ARGP_KEY_ERROR:
            ctx->child_error_calls++;
            return 0;
        case ARGP_KEY_END:
            ctx->child_end_calls++;
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
}

static error_t propagation_parent_parse(int key, char* arg, struct argp_state* state) {
    struct propagation_ctx* ctx = state->input;
    (void)arg;
    switch (key) {
        case ARGP_KEY_INIT:
            state->child_inputs[0] = ctx;
            return 0;
        case ARGP_KEY_ERROR:
            ctx->parent_error_calls++;
            return 0;
        case ARGP_KEY_END:
            ctx->parent_end_calls++;
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
}

static void test_parser_error_propagation_across_parent_child(void) {
    static const struct argp_option child_opts[] = {{"child-fail", 'c', 0, 0, "force child parse failure", 0}, {0}};
    static const struct argp child_argp = {child_opts, propagation_child_parse, NULL, NULL, NULL, NULL, NULL};

    static const struct argp_child children[] = {{&child_argp, 0, NULL, 0}, {0}};
    static const struct argp parent_argp = {empty_options, propagation_parent_parse, NULL, NULL, children, NULL, NULL};

    struct propagation_ctx ctx = {0};
    char prog[] = "prog";
    char fail[] = "--child-fail";
    char* argv[] = {prog, fail, NULL};

    error_t err = argp_parse(&parent_argp, 2, argv, ARGP_NO_EXIT | ARGP_NO_ERRS, NULL, &ctx);
    assert(err == ECANCELED);
    assert(ctx.parent_error_calls > 0);
    assert(ctx.child_error_calls > 0);
    assert(ctx.parent_end_calls == 0);
    assert(ctx.child_end_calls == 0);
}

int main(void) {
    test_argp_input_used_by_help_filter();
    test_argp_usage_contract_without_exit();
    test_negative_parse_cases();
    test_argument_ordering_and_permutation_modes();
    test_parser_error_propagation_across_parent_child();
    return 0;
}
