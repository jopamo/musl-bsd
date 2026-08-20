#include <dlfcn.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef double (*atof_function)(const char* string);

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "atof ABI test failed at line %d\n", __LINE__); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

static int verify_provider(atof_function function) {
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "atof") == (void*)function);
    return 0;
}

int main(void) {
    atof_function function = atof;
    double result;

    CHECK(verify_provider(function) == 0);

    errno = EDOM;
    CHECK(function("  -12.5xyz") == -12.5);
    CHECK(errno == EDOM);
    errno = ERANGE;
    CHECK(function("0x1.8p+2tail") == 6.0);
    CHECK(errno == ERANGE);
    errno = E2BIG;
    CHECK(isinf(function("inf!")));
    CHECK(errno == E2BIG);
    errno = ENOTTY;
    result = function("-0");
    CHECK(result == 0.0);
    CHECK(signbit(result) != 0);
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    CHECK(isnan(function("nan(payload)")));
    CHECK(errno == ENOTTY);

    errno = ENOTTY;
    result = function("not-a-number");
    CHECK(result == 0.0);
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = function("");
    CHECK(result == 0.0);
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = function("1e999");
    CHECK(isinf(result));
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = function("1e-999");
    CHECK(result == 0.0);
    CHECK(errno == ENOTTY);
    return 0;
}
