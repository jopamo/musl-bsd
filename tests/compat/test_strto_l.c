#include <dlfcn.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern float __strtof_l(const char* string, char** end, locale_t locale);
extern double __strtod_l(const char* string, char** end, locale_t locale);

typedef float (*strtof_l_function)(const char* string, char** end, locale_t locale);
typedef double (*strtod_l_function)(const char* string, char** end, locale_t locale);

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "strto_l ABI test failed at line %d\n", __LINE__); \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int verify_provider(const char* internal_name, const char* public_name, const void* function) {
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr(function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, internal_name) == function);
    CHECK(dlsym(RTLD_DEFAULT, public_name) == function);
    return 0;
}

static int verify_successful_formats(strtod_l_function function, locale_t locale) {
    char* end;

    errno = EDOM;
    CHECK(function("  -12.5xyz", &end, locale) == -12.5);
    CHECK(strcmp(end, "xyz") == 0);
    CHECK(errno == EDOM);

    errno = ERANGE;
    CHECK(function("0x1.8p+2tail", &end, locale) == 6.0);
    CHECK(strcmp(end, "tail") == 0);
    CHECK(errno == ERANGE);

    errno = E2BIG;
    CHECK(isinf(function("inf!", &end, locale)));
    CHECK(strcmp(end, "!") == 0);
    CHECK(errno == E2BIG);

    errno = ENOTTY;
    CHECK(isnan(function("nan(payload)", &end, locale)));
    CHECK(*end == '\0');
    CHECK(errno == ENOTTY);
    return 0;
}

static int verify_float_formats(strtof_l_function function, locale_t locale) {
    char* end;

    errno = EDOM;
    CHECK(function("  -12.5xyz", &end, locale) == -12.5f);
    CHECK(strcmp(end, "xyz") == 0);
    CHECK(errno == EDOM);

    errno = ERANGE;
    CHECK(function("0x1.8p+2tail", &end, locale) == 6.0f);
    CHECK(strcmp(end, "tail") == 0);
    CHECK(errno == ERANGE);

    errno = E2BIG;
    CHECK(isinf(function("1e99", &end, locale)));
    CHECK(strcmp(end, "") == 0);
    CHECK(errno == ERANGE);

    errno = ENOTTY;
    CHECK(function("not-a-number", &end, locale) == 0.0f);
    CHECK(*end == 'n');
    CHECK(errno == EINVAL);
    return 0;
}

static int verify_range_and_failure(strtod_l_function function, locale_t locale) {
    const char invalid[] = "not-a-number";
    char* end;

    errno = ENOSPC;
    CHECK(isinf(function("1e999", &end, locale)));
    CHECK(strcmp(end, "") == 0);
    CHECK(errno == ERANGE);

    errno = EBUSY;
    CHECK(function("1e-999", &end, locale) == 0.0);
    CHECK(strcmp(end, "") == 0);
    CHECK(errno == ERANGE);

    errno = ECHILD;
    CHECK(function(invalid, &end, locale) == 0.0);
    CHECK(end == invalid);
    CHECK(errno == EINVAL);
    return 0;
}

int main(void) {
    strtof_l_function float_function = __strtof_l;
    strtod_l_function function = __strtod_l;
    locale_t c_locale;
    locale_t utf8_locale;

    CHECK(verify_provider("__strtof_l", "strtof_l", (const void*)float_function) == 0);
    CHECK(verify_provider("__strtod_l", "strtod_l", (const void*)function) == 0);

    c_locale = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    CHECK(c_locale != (locale_t)0);
    utf8_locale = newlocale(LC_ALL_MASK, "C.UTF-8", (locale_t)0);
    CHECK(utf8_locale != (locale_t)0);

    CHECK(verify_successful_formats(function, c_locale) == 0);
    CHECK(verify_successful_formats(function, utf8_locale) == 0);
    CHECK(verify_float_formats(float_function, c_locale) == 0);
    CHECK(verify_float_formats(float_function, utf8_locale) == 0);
    CHECK(verify_range_and_failure(function, c_locale) == 0);

    freelocale(utf8_locale);
    freelocale(c_locale);
    return 0;
}
