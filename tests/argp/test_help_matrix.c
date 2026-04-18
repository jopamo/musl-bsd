#include <argp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    (void)key;
    (void)arg;
    (void)state;
    return ARGP_ERR_UNKNOWN;
}

static char* render_help(const struct argp* argp, unsigned flags) {
    char* buf = NULL;
    size_t len = 0;
    FILE* stream = open_memstream(&buf, &len);
    assert(stream != NULL);

    argp_help(argp, stream, flags, "prog");
    assert(fclose(stream) == 0);
    assert(buf != NULL);
    return buf;
}

static char* render_state_help(const struct argp* argp, unsigned flags) {
    char* buf = NULL;
    size_t len = 0;
    FILE* stream = open_memstream(&buf, &len);
    assert(stream != NULL);

    struct argp_state state;
    memset(&state, 0, sizeof(state));
    state.root_argp = argp;
    state.flags = ARGP_NO_EXIT;
    state.name = "prog";
    state.err_stream = stream;
    state.out_stream = stream;

    argp_state_help(&state, stream, flags);
    assert(fclose(stream) == 0);
    assert(buf != NULL);
    return buf;
}

static const char* must_find(const char* haystack, const char* needle) {
    const char* p = strstr(haystack, needle);
    assert(p != NULL);
    return p;
}

static int count_substr(const char* haystack, const char* needle) {
    int count = 0;
    const char* p = haystack;
    size_t n = strlen(needle);
    while ((p = strstr(p, needle)) != NULL) {
        count++;
        p += n;
    }
    return count;
}

static void test_argp_state_help_modes(void) {
    static const struct argp_option mode_opts[] = {{"verbose", 'v', 0, 0, "verbose output", 0}, {0}};

    static const struct argp mode_argp = {mode_opts, parse_opt, "FILE", "pre section\vpost section", NULL, NULL, NULL};

    char* usage = render_state_help(&mode_argp, ARGP_HELP_USAGE);
    assert(strstr(usage, "Usage: prog") != NULL);
    assert(strstr(usage, "pre section") == NULL);
    free(usage);

    char* short_usage = render_state_help(&mode_argp, ARGP_HELP_SHORT_USAGE);
    assert(strstr(short_usage, "[OPTION...]") != NULL);
    free(short_usage);

    char* long_no_doc = render_state_help(&mode_argp, ARGP_HELP_LONG);
    assert(strstr(long_no_doc, "--verbose") != NULL);
    assert(strstr(long_no_doc, "pre section") == NULL);
    assert(strstr(long_no_doc, "post section") == NULL);
    free(long_no_doc);

    char* long_with_doc = render_state_help(&mode_argp, ARGP_HELP_LONG | ARGP_HELP_DOC);
    assert(strstr(long_with_doc, "--verbose") != NULL);
    assert(strstr(long_with_doc, "pre section") != NULL);
    assert(strstr(long_with_doc, "post section") != NULL);
    free(long_with_doc);
}

static void test_doc_option_alias_duplicate_empty_cases(void) {
    static const struct argp_option child_opts[] = {
        {"child-opt", 'c', 0, 0, "child option", 0}, {"child doc", 0, 0, OPTION_DOC, "child doc text", 0}, {0}};
    static const struct argp child_argp = {child_opts, parse_opt, NULL, NULL, NULL, NULL, NULL};

    static const struct argp_option opts[] = {{"first", 'f', 0, 0, "first option", 0},
                                              {"first-shadow", 'f', 0, 0, "shadowed short", 0},
                                              {"alias-target", 'a', "VAL", 0, "alias target", 0},
                                              {"alias-long", 0, 0, OPTION_ALIAS, 0, 0},
                                              {"plain doc", 0, 0, OPTION_DOC, "plain documentation entry", 0},
                                              {"--doc-like", 0, 0, OPTION_DOC, "dash-prefixed documentation entry", 0},
                                              {"empty-doc", 'e', 0, 0, "", 0},
                                              {0}};
    static const struct argp_child children[] = {{&child_argp, 0, "Child Docs", 1}, {0}};
    static const struct argp argp = {opts, parse_opt, NULL, NULL, children, NULL, NULL};

    char* out = render_help(&argp, ARGP_HELP_USAGE | ARGP_HELP_LONG | ARGP_HELP_DOC);

    const char* alias_line = must_find(out, "--alias-target=VAL, --alias-long=VAL");
    (void)alias_line;

    must_find(out, "--first-shadow");
    must_find(out, "-f, --first");
    assert(strstr(out, "-f, --first-shadow") == NULL);

    must_find(out, "-e, --empty-doc\n");

    const char* dash_doc = must_find(out, "--doc-like");
    const char* plain_doc = must_find(out, "plain doc");
    assert(dash_doc < plain_doc);

    must_find(out, "Child Docs");
    must_find(out, "child doc                  child doc text");

    free(out);
}

int main(void) {
    static const struct argp_option grand_opts[] = {{"grand-opt", 'G', 0, 0, "grandchild option", 0}, {0}};
    static const struct argp grand_argp = {grand_opts, parse_opt, NULL, NULL, NULL, NULL, NULL};

    static const struct argp_option child_one_opts[] = {
        {"child-z", 0, 0, 0, "child z option", 0}, {"child-a", 'a', 0, 0, "child a option", 0}, {0}};
    static const struct argp_child child_one_children[] = {{&grand_argp, 0, "Grand Cluster", 1}, {0}};
    static const struct argp child_one_argp = {child_one_opts, parse_opt, NULL, NULL, child_one_children, NULL, NULL};

    static const struct argp_option child_two_opts[] = {{"child-neg", 'n', 0, 0, "negative group child option", 0},
                                                        {0}};
    static const struct argp child_two_argp = {child_two_opts, parse_opt, NULL, NULL, NULL, NULL, NULL};

    static const struct argp_option root_opts[] = {{"beta", 'b', 0, 0, "beta option", 0},
                                                   {"Alpha", 0, 0, 0, "alpha long-only option", 0},
                                                   {"--pseudo-doc", 0, 0, OPTION_DOC, "looks like an option", 0},
                                                   {"plain doc", 0, 0, OPTION_DOC, "documentation-only entry", 0},
                                                   {0, 0, 0, 0, "Parent Group:", 0},
                                                   {"gamma", 'g', "ARG", 0, "gamma takes an arg", 0},
                                                   {0}};

    static const struct argp_child root_children[] = {
        {&child_one_argp, 0, "Child One", 2}, {&child_two_argp, 0, "Child Two", -1}, {0}};

    static const struct argp root_argp = {root_opts, parse_opt, "ARG", "root documentation", root_children, NULL, NULL};

    assert(setenv("ARGP_HELP_FMT", "dup-args,no-dup-args-note,rmargin=120", 1) == 0);

    char* out = render_help(&root_argp, ARGP_HELP_USAGE | ARGP_HELP_LONG | ARGP_HELP_DOC);

    const char* long_help = must_find(out, "\nroot documentation\n");

    const char* alpha = must_find(long_help, "--Alpha");
    const char* beta = must_find(long_help, "--beta");
    assert(alpha < beta);

    const char* pseudo_doc = must_find(long_help, "--pseudo-doc");
    const char* plain_doc = must_find(long_help, "plain doc");
    assert(pseudo_doc < plain_doc);

    const char* parent_group = must_find(long_help, "Parent Group:");
    const char* gamma = must_find(long_help, "-g ARG, --gamma=ARG");
    assert(parent_group < gamma);

    const char* child_one = must_find(long_help, "Child One");
    const char* grand_cluster = must_find(long_help, "Grand Cluster");
    const char* child_two = must_find(long_help, "Child Two");
    assert(child_one < grand_cluster);
    assert(child_one < child_two);

    assert(count_substr(out, "Child One") == 1);
    assert(count_substr(out, "Grand Cluster") == 1);

    free(out);

    test_argp_state_help_modes();
    test_doc_option_alias_duplicate_empty_cases();

    return 0;
}
