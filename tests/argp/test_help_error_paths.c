#include <argp.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    (void)key;
    (void)arg;
    (void)state;
    return ARGP_ERR_UNKNOWN;
}

static FILE* open_capture(char** out_buf, size_t* out_len) {
    *out_buf = NULL;
    *out_len = 0;
    FILE* stream = open_memstream(out_buf, out_len);
    assert(stream != NULL);
    return stream;
}

static struct argp_state make_state(const struct argp* root, FILE* err_stream, unsigned flags) {
    struct argp_state state;
    memset(&state, 0, sizeof(state));
    state.root_argp = root;
    state.flags = flags;
    state.name = "prog";
    state.err_stream = err_stream;
    state.out_stream = err_stream;
    return state;
}

static void test_argp_error_emits_context_and_help_hint(const struct argp* root) {
    char* out = NULL;
    size_t out_len = 0;
    FILE* stream = open_capture(&out, &out_len);

    struct argp_state state = make_state(root, stream, ARGP_NO_EXIT);
    argp_error(&state, "invalid value: %s", "bad");

    assert(fclose(stream) == 0);
    assert(strstr(out, "prog: invalid value: bad") != NULL);
    assert(strstr(out, "Try `prog --help' or `prog --usage' for more information.") != NULL);

    free(out);
}

static void test_argp_failure_formats_message_and_errno(const struct argp* root) {
    char* out = NULL;
    size_t out_len = 0;
    FILE* stream = open_capture(&out, &out_len);

    struct argp_state state = make_state(root, stream, ARGP_NO_EXIT);

    argp_failure(&state, 17, ENOENT, "cannot open %s", "file.txt");
    argp_failure(&state, 0, EPERM, NULL);

    assert(fclose(stream) == 0);

    assert(strstr(out, "prog: cannot open file.txt: No such file or directory") != NULL);
    assert(strstr(out, "prog: Operation not permitted") != NULL);

    free(out);
}

int main(void) {
    static const struct argp_option options[] = {{"verbose", 'v', 0, 0, "verbose output", 0}, {0}};
    static const struct argp argp = {options, parse_opt, NULL, NULL, NULL, NULL, NULL};

    test_argp_error_emits_context_and_help_hint(&argp);
    test_argp_failure_formats_message_and_errno(&argp);
    return 0;
}
