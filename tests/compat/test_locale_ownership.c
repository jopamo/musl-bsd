#include <dlfcn.h>
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

extern int __strcoll_l(const char* left, const char* right, locale_t locale);
extern size_t __strxfrm_l(char* destination, const char* source, size_t size, locale_t locale);
extern int __wcscoll_l(const wchar_t* left, const wchar_t* right, locale_t locale);
extern size_t __wcsxfrm_l(wchar_t* destination, const wchar_t* source, size_t size, locale_t locale);
extern locale_t __newlocale(int mask, const char* name, locale_t locale);
extern char* __nl_langinfo_l(nl_item item, locale_t locale);
extern locale_t __duplocale(locale_t locale);
extern void __freelocale(locale_t locale);
extern locale_t __uselocale(locale_t locale);

typedef int (*strcoll_l_function)(const char* left, const char* right, locale_t locale);
typedef size_t (*strxfrm_l_function)(char* destination, const char* source, size_t size, locale_t locale);
typedef int (*wcscoll_l_function)(const wchar_t* left, const wchar_t* right, locale_t locale);
typedef size_t (*wcsxfrm_l_function)(wchar_t* destination, const wchar_t* source, size_t size, locale_t locale);

#define CHECK(condition)                                                            \
    do {                                                                            \
        if (!(condition)) {                                                         \
            fprintf(stderr, "locale ownership test failed at line %d\n", __LINE__); \
            return 1;                                                               \
        }                                                                           \
    } while (0)

static int sign_of(int value) {
    return (value > 0) - (value < 0);
}

static int verify_collation(strcoll_l_function function, locale_t locale) {
    static const struct {
        const char* left;
        const char* right;
        int expected_sign;
    } cases[] = {
        {"", "", 0}, {"a", "a", 0}, {"a", "b", -1}, {"b", "a", 1}, {"aa", "b", -1}, {"b", "aa", 1}, {"a\0z", "a\0a", 0},
    };
    size_t index;

    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        errno = EDOM;
        CHECK(sign_of(function(cases[index].left, cases[index].right, locale)) == cases[index].expected_sign);
        CHECK(errno == EDOM);
    }
    return 0;
}

static int verify_transformation(strxfrm_l_function function, locale_t locale) {
    static const char source[] = "alpha\0ignored";
    char output[16];
    char truncated[4];

    errno = EDOM;
    memset(output, 0, sizeof(output));
    CHECK(function(output, source, sizeof(output), locale) == 5);
    CHECK(strcmp(output, "alpha") == 0);
    CHECK(errno == EDOM);

    errno = ERANGE;
    memset(truncated, 0xA5, sizeof(truncated));
    CHECK(function(truncated, source, sizeof(truncated), locale) == 5);
    CHECK(memcmp(truncated, "\xA5\xA5\xA5\xA5", sizeof(truncated)) == 0);
    CHECK(errno == ERANGE);

    errno = E2BIG;
    memset(output, 0xA5, sizeof(output));
    CHECK(function(output, source, 0, locale) == 5);
    CHECK(memcmp(output, "\xA5\xA5\xA5\xA5", sizeof(truncated)) == 0);
    CHECK(errno == E2BIG);

    errno = ENOTTY;
    memset(output, 0xA5, sizeof(output));
    CHECK(function(output, "", sizeof(output), locale) == 0);
    CHECK(output[0] == '\0');
    CHECK(errno == ENOTTY);
    return 0;
}

static int verify_wide_collation(wcscoll_l_function function, locale_t locale) {
    static const struct {
        const wchar_t* left;
        const wchar_t* right;
        int expected_sign;
    } cases[] = {
        {L"", L"", 0}, {L"alpha", L"alpha", 0}, {L"alpha", L"beta", -1}, {L"beta", L"alpha", 1}, {L"\u00E4", L"z", 1},
    };
    size_t index;

    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        errno = EDOM;
        CHECK(sign_of(function(cases[index].left, cases[index].right, locale)) == cases[index].expected_sign);
        CHECK(errno == EDOM);
    }
    return 0;
}

static int verify_wide_transformation(wcsxfrm_l_function function, locale_t locale) {
    static const wchar_t source[] = L"alpha\u00E4\0ignored";
    wchar_t output[16];
    wchar_t truncated[4];
    wchar_t untouched[16];

    errno = EDOM;
    wmemset(output, 0, sizeof(output) / sizeof(output[0]));
    CHECK(function(output, source, sizeof(output) / sizeof(output[0]), locale) == 6);
    CHECK(wmemcmp(output, source, 7) == 0);
    CHECK(errno == EDOM);

    errno = ERANGE;
    wmemset(truncated, 0xA5, sizeof(truncated) / sizeof(truncated[0]));
    CHECK(function(truncated, source, sizeof(truncated) / sizeof(truncated[0]), locale) == 6);
    CHECK(wmemcmp(truncated, L"alp", 3) == 0);
    CHECK(truncated[3] == L'\0');
    CHECK(errno == ERANGE);

    errno = E2BIG;
    wmemset(output, 0xA5, sizeof(output) / sizeof(output[0]));
    wmemset(untouched, 0xA5, sizeof(untouched) / sizeof(untouched[0]));
    CHECK(function(output, source, 0, locale) == 6);
    CHECK(wmemcmp(output, untouched, sizeof(output) / sizeof(output[0])) == 0);
    CHECK(errno == E2BIG);

    errno = ENOTTY;
    wmemset(output, 0xA5, sizeof(output) / sizeof(output[0]));
    CHECK(function(output, L"", sizeof(output) / sizeof(output[0]), locale) == 0);
    CHECK(output[0] == L'\0');
    CHECK(errno == ENOTTY);
    return 0;
}

int main(void) {
    static const nl_item gpucomp_items[] = {
        0x40000, 0x40002, 0x40003, 0x40004, 0x40005, 0x40006, 0x40015,
    };
    strcoll_l_function compare_locale = __strcoll_l;
    strxfrm_l_function transform_locale = __strxfrm_l;
    wcscoll_l_function wide_compare_locale = __wcscoll_l;
    wcsxfrm_l_function wide_transform_locale = __wcsxfrm_l;
    char* (*query_langinfo)(nl_item, locale_t) = __nl_langinfo_l;
    char* result;
    locale_t (*create_locale)(int, const char*, locale_t) = __newlocale;
    locale_t (*duplicate_locale)(locale_t) = __duplocale;
    void (*free_locale)(locale_t) = __freelocale;
    locale_t (*select_locale)(locale_t) = __uselocale;
    locale_t active_before;
    locale_t c_locale;
    locale_t global_c_snapshot;
    locale_t global_utf8_snapshot;
    locale_t owned_source;
    locale_t observed_locale;
    locale_t previous;
    locale_t snapshot;
    locale_t utf8_locale;
    Dl_info info;
    size_t index;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)compare_locale, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__strcoll_l") == (void*)compare_locale);
    CHECK(dlsym(RTLD_DEFAULT, "strcoll_l") == (void*)compare_locale);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)transform_locale, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__strxfrm_l") == (void*)transform_locale);
    CHECK(dlsym(RTLD_DEFAULT, "strxfrm_l") == (void*)transform_locale);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)wide_compare_locale, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__wcscoll_l") == (void*)wide_compare_locale);
    CHECK(dlsym(RTLD_DEFAULT, "wcscoll_l") == (void*)wide_compare_locale);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)wide_transform_locale, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__wcsxfrm_l") == (void*)wide_transform_locale);
    CHECK(dlsym(RTLD_DEFAULT, "wcsxfrm_l") == (void*)wide_transform_locale);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)query_langinfo, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__nl_langinfo_l") == (void*)query_langinfo);
    CHECK(dlsym(RTLD_DEFAULT, "nl_langinfo_l") == (void*)query_langinfo);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)create_locale, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__newlocale") == (void*)create_locale);
    CHECK(dlsym(RTLD_DEFAULT, "newlocale") == (void*)create_locale);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)duplicate_locale, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__duplocale") == (void*)duplicate_locale);
    CHECK(dlsym(RTLD_DEFAULT, "duplocale") == (void*)duplicate_locale);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)free_locale, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__freelocale") == (void*)free_locale);
    CHECK(dlsym(RTLD_DEFAULT, "freelocale") == (void*)free_locale);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)select_locale, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__uselocale") == (void*)select_locale);
    CHECK(dlsym(RTLD_DEFAULT, "uselocale") == (void*)select_locale);

    active_before = uselocale((locale_t)0);
    CHECK(active_before != (locale_t)0);
    errno = EDOM;
    c_locale = create_locale(LC_ALL_MASK, "C", (locale_t)0);
    CHECK(c_locale != (locale_t)0);
    CHECK(errno == EDOM);
    errno = ERANGE;
    utf8_locale = create_locale(LC_ALL_MASK, "C.UTF-8", (locale_t)0);
    CHECK(utf8_locale != (locale_t)0);
    CHECK(errno == ERANGE);
    CHECK(uselocale((locale_t)0) == active_before);

    errno = E2BIG;
    CHECK(select_locale(c_locale) == active_before);
    CHECK(errno == E2BIG);
    errno = ECHILD;
    CHECK(select_locale((locale_t)0) == c_locale);
    CHECK(errno == ECHILD);
    errno = ENOTTY;
    CHECK(select_locale(active_before) == c_locale);
    CHECK(errno == ENOTTY);

    CHECK(verify_collation(compare_locale, c_locale) == 0);
    CHECK(verify_collation(compare_locale, utf8_locale) == 0);
    CHECK(verify_transformation(transform_locale, c_locale) == 0);
    CHECK(verify_transformation(transform_locale, utf8_locale) == 0);
    CHECK(verify_wide_collation(wide_compare_locale, c_locale) == 0);
    CHECK(verify_wide_collation(wide_compare_locale, utf8_locale) == 0);
    CHECK(verify_wide_transformation(wide_transform_locale, c_locale) == 0);
    CHECK(verify_wide_transformation(wide_transform_locale, utf8_locale) == 0);

    for (index = 0; index < sizeof(gpucomp_items) / sizeof(gpucomp_items[0]); ++index) {
        errno = EDOM;
        result = query_langinfo(gpucomp_items[index], c_locale);
        CHECK(result != NULL);
        CHECK(result[0] == '\0');
        CHECK(errno == EDOM);
    }
    errno = ERANGE;
    CHECK(strcmp(query_langinfo(CODESET, c_locale), "ASCII") == 0);
    CHECK(errno == ERANGE);
    errno = EAGAIN;
    CHECK(strcmp(query_langinfo(CODESET, utf8_locale), "UTF-8") == 0);
    CHECK(errno == EAGAIN);

    /*
     * Gpucomp's compiled call site passes mask 0x40, "C", and a null base
     * locale. Keep that ABI path covered even though it is outside musl's
     * standard LC_* mask range.
     */
    errno = ECHILD;
    observed_locale = create_locale(0x40, "C", (locale_t)0);
    CHECK(observed_locale != (locale_t)0);
    CHECK(errno == ECHILD);
    CHECK(uselocale((locale_t)0) == active_before);
    errno = ENOEXEC;
    free_locale(observed_locale);
    CHECK(errno == ENOEXEC);

    errno = EDOM;
    owned_source = duplicate_locale(utf8_locale);
    CHECK(owned_source != (locale_t)0);
    CHECK(errno == EDOM);
    errno = ERANGE;
    snapshot = duplicate_locale(owned_source);
    CHECK(snapshot != (locale_t)0);
    CHECK(errno == ERANGE);
    CHECK(uselocale((locale_t)0) == active_before);

    errno = E2BIG;
    CHECK(create_locale(LC_CTYPE_MASK, "C", owned_source) == owned_source);
    CHECK(errno == E2BIG);
    CHECK(strcmp(query_langinfo(CODESET, owned_source), "ASCII") == 0);
    CHECK(strcmp(query_langinfo(CODESET, snapshot), "UTF-8") == 0);
    errno = E2BIG;
    free_locale(owned_source);
    CHECK(errno == E2BIG);

    previous = uselocale(snapshot);
    CHECK(previous == active_before);
    CHECK(MB_CUR_MAX == 4);
    CHECK(strcmp(nl_langinfo(CODESET), "UTF-8") == 0);
    CHECK(uselocale(previous) == snapshot);
    errno = ENOTTY;
    free_locale(snapshot);
    CHECK(errno == ENOTTY);

    CHECK(setlocale(LC_ALL, "C") != NULL);
    errno = E2BIG;
    global_c_snapshot = duplicate_locale(LC_GLOBAL_LOCALE);
    CHECK(global_c_snapshot != (locale_t)0);
    CHECK(errno == E2BIG);
    CHECK(strcmp(query_langinfo(CODESET, global_c_snapshot), "ASCII") == 0);

    CHECK(setlocale(LC_ALL, "C.UTF-8") != NULL);
    CHECK(strcmp(query_langinfo(CODESET, global_c_snapshot), "ASCII") == 0);
    errno = EAGAIN;
    global_utf8_snapshot = duplicate_locale(LC_GLOBAL_LOCALE);
    CHECK(global_utf8_snapshot != (locale_t)0);
    CHECK(errno == EAGAIN);
    CHECK(strcmp(query_langinfo(CODESET, global_utf8_snapshot), "UTF-8") == 0);

    errno = ECHILD;
    free_locale(global_utf8_snapshot);
    CHECK(errno == ECHILD);
    errno = ENOEXEC;
    free_locale(global_c_snapshot);
    CHECK(errno == ENOEXEC);

    errno = ENOSPC;
    free_locale(utf8_locale);
    CHECK(errno == ENOSPC);
    errno = EPIPE;
    free_locale(c_locale);
    CHECK(errno == EPIPE);
    return 0;
}
