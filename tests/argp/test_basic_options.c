#include <argp.h>
#include <assert.h>
#include <string.h>

struct parse_result {
    int verbose_seen;
    const char* input;
    unsigned arg_count;
    const char* last_arg;
    unsigned arg_num_on_last;
    int saw_end;
    int saw_success;
    int saw_no_args;
};

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    struct parse_result* res = state->input;

    switch (key) {
        case 'v':
            res->verbose_seen++;
            break;
        case 'i':
            res->input = arg;
            break;
        case ARGP_KEY_ARG:
            res->last_arg = arg;
            res->arg_num_on_last = state->arg_num;
            res->arg_count++;
            break;
        case ARGP_KEY_NO_ARGS:
            res->saw_no_args = 1;
            break;
        case ARGP_KEY_END:
            res->saw_end = 1;
            break;
        case ARGP_KEY_SUCCESS:
            res->saw_success = 1;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

int main(void) {
    static const struct argp_option options[] = {
        {"verbose", 'v', 0, 0, "enable verbose output", 0},
        {"input", 'i', "FILE", 0, "input file path", 0},
        {0}
    };

    static const struct argp argp = {options, parse_opt, "ARG", "basic argp option handling", NULL, NULL, NULL};

    char prog[] = "prog";
    char verbose[] = "--verbose";
    char input[] = "--input=file.txt";
    char positional[] = "payload";
    char* argv[] = {prog, verbose, input, positional, NULL};
    int argc = 4;

    struct parse_result res = {0};

    error_t err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, NULL, &res);
    assert(err == 0);
    assert(res.verbose_seen == 1);
    assert(res.input != NULL && strcmp(res.input, "file.txt") == 0);
    assert(res.arg_count == 1);
    assert(res.arg_num_on_last == 0);
    assert(strcmp(res.last_arg, "payload") == 0);
    assert(res.saw_end == 1);
    assert(res.saw_success == 1);
    assert(res.saw_no_args == 0);

    return 0;
}
