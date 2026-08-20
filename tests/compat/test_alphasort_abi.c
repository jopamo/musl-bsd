#include <dlfcn.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

typedef int (*alphasort_function)(const struct dirent** left, const struct dirent** right);

#define CHECK(condition)                                                         \
    do {                                                                         \
        if (!(condition)) {                                                      \
            fprintf(stderr, "alphasort ABI test failed at line %d\n", __LINE__); \
            return 1;                                                            \
        }                                                                        \
    } while (0)

static int sign_of(int value) {
    return (value > 0) - (value < 0);
}

static int verify_order(alphasort_function function, locale_t locale) {
    static const struct {
        const char* left;
        const char* right;
        int expected_sign;
    } cases[] = {
        {"alpha", "beta", -1}, {"beta", "alpha", 1}, {"same", "same", 0}, {"a9", "a10", 1}, {"\xC3\xA4", "z", 1},
    };
    struct dirent left;
    struct dirent right;
    const struct dirent* left_pointer = &left;
    const struct dirent* right_pointer = &right;
    size_t index;

    CHECK(uselocale(locale) != (locale_t)0);
    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        memset(&left, 0, sizeof(left));
        memset(&right, 0, sizeof(right));
        strcpy(left.d_name, cases[index].left);
        strcpy(right.d_name, cases[index].right);
        errno = EDOM;
        CHECK(sign_of(function(&left_pointer, &right_pointer)) == cases[index].expected_sign);
        CHECK(errno == EDOM);
    }
    return 0;
}

int main(void) {
    alphasort_function function = alphasort;
    locale_t c_locale;
    locale_t previous;
    locale_t utf8_locale;
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "alphasort") == (void*)function);

    previous = uselocale((locale_t)0);
    CHECK(previous != (locale_t)0);
    c_locale = newlocale(LC_COLLATE_MASK, "C", (locale_t)0);
    CHECK(c_locale != (locale_t)0);
    utf8_locale = newlocale(LC_COLLATE_MASK, "C.UTF-8", (locale_t)0);
    CHECK(utf8_locale != (locale_t)0);

    CHECK(verify_order(function, c_locale) == 0);
    CHECK(verify_order(function, utf8_locale) == 0);
    CHECK(uselocale(previous) == utf8_locale);
    freelocale(utf8_locale);
    freelocale(c_locale);
    return 0;
}
