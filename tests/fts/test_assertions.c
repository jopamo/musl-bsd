#include "test_support.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define C_RED "\033[31m"
#define C_GRN "\033[32m"
#define C_YEL "\033[33m"
#define C_RST "\033[0m"

int fts_test_failures = 0;
int fts_test_strict = 0;

static void vcheck_log(int cond, int hard, const char* fmt, va_list ap) {
    const char* color = cond ? C_GRN : (hard ? C_RED : C_YEL);
    const char* tag = cond ? "[ ok ]" : (hard ? "[fail]" : "[warn]");

    fprintf(stderr, "%s%s " C_RST, color, tag);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

void fts_set_strict_from_env(void) {
    const char* env = getenv("FTS_TEST_STRICT");
    fts_test_strict = env && strcmp(env, "0") != 0;
    fts_test_failures = 0;
}

void fts_check(int cond, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vcheck_log(cond, 1, fmt, ap);
    va_end(ap);
    if (!cond)
        fts_test_failures++;
}

void fts_check_soft(int cond, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vcheck_log(cond, fts_test_strict, fmt, ap);
    va_end(ap);
    if (!cond && fts_test_strict)
        fts_test_failures++;
}

int fts_exit_code(void) {
    if (fts_test_failures) {
        fprintf(stderr, C_RED "\n%d checks failed\n" C_RST, fts_test_failures);
        return 1;
    }
    fprintf(stderr, C_GRN "\nall checks passed\n" C_RST);
    return 0;
}

