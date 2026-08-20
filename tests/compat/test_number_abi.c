#include <dlfcn.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef double (*atof_function)(const char* string);
typedef int (*atoi_function)(const char* string);

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "number ABI test failed at line %d\n", __LINE__); \
            return 1;                                                         \
        }                                                                     \
    } while (0)

static int verify_provider(const char* name, const void* function) {
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, name) == function);
    return 0;
}

int main(void) {
    atof_function atof_function_value = atof;
    atoi_function atoi_function_value = atoi;
    double result;

    CHECK(verify_provider("atof", (const void*)atof_function_value) == 0);
    CHECK(verify_provider("atoi", (const void*)atoi_function_value) == 0);

    errno = EDOM;
    CHECK(atof_function_value("  -12.5xyz") == -12.5);
    CHECK(errno == EDOM);
    errno = ERANGE;
    CHECK(atof_function_value("0x1.8p+2tail") == 6.0);
    CHECK(errno == ERANGE);
    errno = E2BIG;
    CHECK(isinf(atof_function_value("inf!")));
    CHECK(errno == E2BIG);
    errno = ENOTTY;
    result = atof_function_value("-0");
    CHECK(result == 0.0);
    CHECK(signbit(result) != 0);
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    CHECK(isnan(atof_function_value("nan(payload)")));
    CHECK(errno == ENOTTY);

    errno = ENOTTY;
    result = atof_function_value("not-a-number");
    CHECK(result == 0.0);
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = atof_function_value("");
    CHECK(result == 0.0);
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = atof_function_value("1e999");
    CHECK(isinf(result));
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = atof_function_value("1e-999");
    CHECK(result == 0.0);
    CHECK(errno == ENOTTY);

    errno = EDOM;
    CHECK(atoi_function_value("  -123xyz") == -123);
    CHECK(errno == EDOM);
    errno = ERANGE;
    CHECK(atoi_function_value("+42") == 42);
    CHECK(errno == ERANGE);
    errno = ENOTTY;
    CHECK(atoi_function_value("not-a-number") == 0);
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    CHECK(atoi_function_value("") == 0);
    CHECK(errno == ENOTTY);
    return 0;
}
