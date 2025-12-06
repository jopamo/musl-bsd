#include <argp.h>
#include <assert.h>
#include <string.h>

struct parent_state {
    int flag_seen;
    int init_called;
    int end_called;
    int success_called;
    int child_input_set;
};

struct child_state {
    int init_called;
    int seen;
    const char* value;
    int end_called;
    int success_called;
    int input_matches;
};

struct parse_context {
    struct parent_state parent;
    struct child_state child;
};

static error_t child_parse(int key, char* arg, struct argp_state* state) {
    struct child_state* child = state->input;

    switch (key) {
        case 'c':
            child->seen++;
            child->value = arg;
            break;
        case ARGP_KEY_INIT:
            child->init_called++;
            if (state->input == child)
                child->input_matches++;
            break;
        case ARGP_KEY_END:
            child->end_called++;
            break;
        case ARGP_KEY_SUCCESS:
            child->success_called++;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static error_t parent_parse(int key, char* arg, struct argp_state* state) {
    struct parse_context* ctx = state->input;
    struct parent_state* parent = &ctx->parent;

    switch (key) {
        case 'p':
            parent->flag_seen++;
            break;
        case ARGP_KEY_INIT:
            parent->init_called++;
            state->child_inputs[0] = &ctx->child;
            parent->child_input_set++;
            break;
        case ARGP_KEY_END:
            parent->end_called++;
            break;
        case ARGP_KEY_SUCCESS:
            parent->success_called++;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

int main(void) {
    static const struct argp_option child_opts[] = {
        {"child", 'c', "VALUE", 0, "child value", 0},
        {0}
    };
    static const struct argp child_argp = {child_opts, child_parse, NULL, "child parser", NULL, NULL, NULL};

    static const struct argp_option parent_opts[] = {
        {"parent", 'p', 0, 0, "parent flag", 0},
        {0}
    };
    static const struct argp_child children[] = {
        {&child_argp, 0, "child options", 0},
        {0}
    };
    static const struct argp parent_argp = {parent_opts, parent_parse, NULL, "parent parser", children, NULL, NULL};

    char prog[] = "prog";
    char parent_flag[] = "--parent";
    char child_value[] = "--child=abc";
    char* argv[] = {prog, parent_flag, child_value, NULL};
    int argc = 3;

    struct parse_context ctx = {0};

    error_t err = argp_parse(&parent_argp, argc, argv, ARGP_NO_EXIT, NULL, &ctx);
    assert(err == 0);
    assert(ctx.parent.flag_seen == 1);
    assert(ctx.parent.init_called == 1);
    assert(ctx.parent.child_input_set == 1);
    assert(ctx.parent.end_called == 1);
    assert(ctx.parent.success_called == 1);

    assert(ctx.child.init_called == 1);
    assert(ctx.child.input_matches == 1);
    assert(ctx.child.seen == 1);
    assert(strcmp(ctx.child.value, "abc") == 0);
    assert(ctx.child.end_called == 1);
    assert(ctx.child.success_called == 1);

    return 0;
}
