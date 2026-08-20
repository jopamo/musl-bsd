#include <dlfcn.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

extern int __iswctype_l(wint_t character, wctype_t descriptor, locale_t locale);
extern wint_t __towlower_l(wint_t character, locale_t locale);
extern wint_t __towupper_l(wint_t character, locale_t locale);

typedef int (*iswctype_l_function)(wint_t character, wctype_t descriptor, locale_t locale);
typedef wint_t (*tow_conversion_function)(wint_t character, locale_t locale);

struct classification_case {
    const char* name;
    wint_t matching;
    wint_t nonmatching;
};

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "wctype_l test failed at line %d\n", __LINE__); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

static int verify_ascii_classes(iswctype_l_function function, locale_t c_locale, locale_t utf8_locale) {
    static const struct classification_case cases[] = {
        {"alnum", L'A', L'!'}, {"alpha", L'z', L'8'},  {"blank", L' ', L'\n'}, {"cntrl", L'\n', L'A'},
        {"digit", L'7', L'A'}, {"graph", L'!', L' '},  {"lower", L'z', L'Z'},  {"print", L' ', L'\n'},
        {"punct", L'!', L'A'}, {"space", L'\n', L'A'}, {"upper", L'Z', L'z'},  {"xdigit", L'F', L'G'},
    };
    size_t index;

    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        wctype_t descriptor = wctype(cases[index].name);

        CHECK(descriptor != (wctype_t)0);
        errno = EDOM;
        CHECK(function(cases[index].matching, descriptor, c_locale) != 0);
        CHECK(errno == EDOM);
        errno = ERANGE;
        CHECK(function(cases[index].nonmatching, descriptor, c_locale) == 0);
        CHECK(errno == ERANGE);
        errno = E2BIG;
        CHECK(function(cases[index].matching, descriptor, utf8_locale) != 0);
        CHECK(errno == E2BIG);
        errno = ENOTTY;
        CHECK(function(cases[index].nonmatching, descriptor, utf8_locale) == 0);
        CHECK(errno == ENOTTY);
    }
    return 0;
}

static int verify_locale_divergence(iswctype_l_function function, locale_t c_locale, locale_t utf8_locale) {
    wctype_t alpha = wctype("alpha");
    wctype_t lower = wctype("lower");
    wctype_t space = wctype("space");

    CHECK(alpha != (wctype_t)0);
    CHECK(lower != (wctype_t)0);
    CHECK(space != (wctype_t)0);

    errno = EDOM;
    CHECK(function((wint_t)0x00e9, alpha, c_locale) != 0);
    CHECK(function((wint_t)0x00e9, alpha, utf8_locale) != 0);
    CHECK(function((wint_t)0x00e9, lower, c_locale) != 0);
    CHECK(function((wint_t)0x00e9, lower, utf8_locale) != 0);
    CHECK(function((wint_t)0x2003, space, c_locale) != 0);
    CHECK(function((wint_t)0x2003, space, utf8_locale) != 0);
    CHECK(errno == EDOM);

    errno = ERANGE;
    CHECK(function(WEOF, alpha, c_locale) == 0);
    CHECK(function(L'A', (wctype_t)0, c_locale) == 0);
    CHECK(errno == ERANGE);
    return 0;
}

static int verify_case_conversion(tow_conversion_function lower,
                                  tow_conversion_function upper,
                                  locale_t c_locale,
                                  locale_t utf8_locale) {
    static const struct {
        wint_t input;
        wint_t lower;
        wint_t upper;
    } cases[] = {
        {L'A', L'a', L'A'},
        {L'a', L'a', L'A'},
        {0x00C4, 0x00E4, 0x00C4},
        {0x00E4, 0x00E4, 0x00C4},
    };
    size_t index;

    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        errno = EDOM;
        CHECK(lower(cases[index].input, c_locale) == cases[index].lower);
        CHECK(upper(cases[index].input, c_locale) == cases[index].upper);
        CHECK(errno == EDOM);
        errno = ERANGE;
        CHECK(lower(cases[index].input, utf8_locale) == cases[index].lower);
        CHECK(upper(cases[index].input, utf8_locale) == cases[index].upper);
        CHECK(errno == ERANGE);
    }

    errno = E2BIG;
    CHECK(lower(WEOF, c_locale) == WEOF);
    CHECK(upper(WEOF, utf8_locale) == WEOF);
    CHECK(errno == E2BIG);
    return 0;
}

int main(void) {
    iswctype_l_function function = __iswctype_l;
    tow_conversion_function lower = __towlower_l;
    tow_conversion_function upper = __towupper_l;
    locale_t c_locale;
    locale_t previous;
    locale_t utf8_locale;
    Dl_info info;

    _Static_assert(sizeof(wint_t) == 4, "x86_64 glibc wint_t ABI");
    _Static_assert(sizeof(wctype_t) == sizeof(unsigned long), "x86_64 glibc wctype_t ABI");

    c_locale = newlocale(LC_CTYPE_MASK, "C", (locale_t)0);
    CHECK(c_locale != (locale_t)0);
    utf8_locale = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    CHECK(utf8_locale != (locale_t)0);
    previous = uselocale(c_locale);
    CHECK(previous != (locale_t)0);

    CHECK(verify_ascii_classes(function, c_locale, utf8_locale) == 0);
    CHECK(verify_locale_divergence(function, c_locale, utf8_locale) == 0);
    CHECK(uselocale(utf8_locale) == c_locale);
    CHECK(verify_locale_divergence(function, c_locale, utf8_locale) == 0);
    CHECK(uselocale(previous) == utf8_locale);

    CHECK(wctype("not-a-class") == (wctype_t)0);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__iswctype_l") == (void*)function);
    CHECK(dlsym(RTLD_DEFAULT, "iswctype_l") == (void*)function);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)lower, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__towlower_l") == (void*)lower);
    CHECK(dlsym(RTLD_DEFAULT, "towlower_l") == (void*)lower);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)upper, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__towupper_l") == (void*)upper);
    CHECK(dlsym(RTLD_DEFAULT, "towupper_l") == (void*)upper);
    CHECK(verify_case_conversion(lower, upper, c_locale, utf8_locale) == 0);

    freelocale(utf8_locale);
    freelocale(c_locale);
    return 0;
}
