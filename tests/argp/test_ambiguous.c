#include <argp.h>
#include <assert.h>
#include <errno.h>

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    (void)key;
    (void)arg;
    (void)state;
    return 0;
}

int main(void) {
    static const struct argp_option options[] = {
        {"foo", 'f', 0, 0, "foo option", 0},
        {"fob", 'b', 0, 0, "fob option", 0},
        {0}
    };

    static const struct argp argp = {options, parse_opt, NULL, NULL, NULL, NULL, NULL};

    char prog[] = "prog";
    char opt[] = "--fo";
    char* argv[] = {prog, opt, NULL};
    int argc = 2;

    error_t err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT | ARGP_NO_ERRS, NULL, NULL);
    assert(err == EINVAL);
    return 0;
}
