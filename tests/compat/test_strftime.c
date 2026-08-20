#include <dlfcn.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

extern size_t __strftime_l(char* restrict string,
                           size_t size,
                           const char* restrict format,
                           const struct tm* restrict time_value,
                           locale_t locale);

typedef size_t (*strftime_l_function)(char* restrict string,
                                      size_t size,
                                      const char* restrict format,
                                      const struct tm* restrict time_value,
                                      locale_t locale);

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "strftime ABI test failed at line %d\n", __LINE__); \
            return 1;                                                           \
        }                                                                       \
    } while (0)

static int verify_numeric_formats(strftime_l_function function, locale_t locale, const struct tm* time_value) {
    char output[64];

    errno = EDOM;
    memset(output, 0, sizeof(output));
    CHECK(function(output, sizeof(output), "%Y-%m-%d %H:%M:%S", time_value, locale) == 19);
    CHECK(strcmp(output, "2024-03-05 14:06:07") == 0);
    CHECK(errno == EDOM);

    errno = ERANGE;
    memset(output, 0, sizeof(output));
    CHECK(function(output, sizeof(output), "%j %%", time_value, locale) == 5);
    CHECK(strcmp(output, "065 %") == 0);
    CHECK(errno == ERANGE);
    return 0;
}

int main(void) {
    static const struct tm time_value = {
        .tm_sec = 7,
        .tm_min = 6,
        .tm_hour = 14,
        .tm_mday = 5,
        .tm_mon = 2,
        .tm_year = 124,
        .tm_wday = 2,
        .tm_yday = 64,
        .tm_isdst = 0,
    };
    char small[8];
    char output[32];
    strftime_l_function function = __strftime_l;
    locale_t c_locale;
    locale_t utf8_locale;
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") != NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__strftime_l") == (void*)function);

    c_locale = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    CHECK(c_locale != (locale_t)0);
    utf8_locale = newlocale(LC_ALL_MASK, "C.UTF-8", (locale_t)0);
    CHECK(utf8_locale != (locale_t)0);

    CHECK(verify_numeric_formats(function, c_locale, &time_value) == 0);
    CHECK(verify_numeric_formats(function, utf8_locale, &time_value) == 0);

    errno = E2BIG;
    memset(small, 0xA5, sizeof(small));
    CHECK(function(small, sizeof(small), "%Y-%m-%d", &time_value, c_locale) == 0);
    CHECK(errno == E2BIG);

    errno = EINTR;
    memset(output, 0xA5, sizeof(output));
    CHECK(function(output, 0, "%Y", &time_value, c_locale) == 0);
    CHECK(errno == EINTR);

    freelocale(utf8_locale);
    freelocale(c_locale);
    return 0;
}
