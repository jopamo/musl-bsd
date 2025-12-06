#include <argp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    (void)key;
    (void)arg;
    (void)state;
    return 0;
}

int main(void) {
    static const struct argp_option options[] = {
        {"file", 'f', "FILE", 0, "input file", 0},
        {0}
    };
    static const struct argp argp = {options, parse_opt, NULL, "help format test", NULL, NULL, NULL};

    int rc = setenv("ARGP_HELP_FMT", "dup-args,opt-doc-col=20,long-opt-col=8,short-opt-col=2", 1);
    assert(rc == 0);

    char* buf = NULL;
    size_t len = 0;
    FILE* stream = open_memstream(&buf, &len);
    assert(stream != NULL);

    argp_help(&argp, stream, ARGP_HELP_USAGE | ARGP_HELP_LONG | ARGP_HELP_DOC, "prog");
    assert(fclose(stream) == 0);
    assert(buf != NULL);

    assert(strstr(buf, "-f FILE, --file=FILE") != NULL);
    assert(strstr(buf, "input file") != NULL);

    free(buf);
    return 0;
}
