#include <dlfcn.h>
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern locale_t __newlocale(int mask, const char* name, locale_t locale);
extern locale_t __duplocale(locale_t locale);
extern void __freelocale(locale_t locale);

#define CHECK(condition)                                                            \
    do {                                                                            \
        if (!(condition)) {                                                         \
            fprintf(stderr, "locale ownership test failed at line %d\n", __LINE__); \
            return 1;                                                               \
        }                                                                           \
    } while (0)

int main(void) {
    locale_t (*create_locale)(int, const char*, locale_t) = __newlocale;
    locale_t (*duplicate_locale)(locale_t) = __duplocale;
    void (*free_locale)(locale_t) = __freelocale;
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
    CHECK(strcmp(nl_langinfo_l(CODESET, owned_source), "ASCII") == 0);
    CHECK(strcmp(nl_langinfo_l(CODESET, snapshot), "UTF-8") == 0);
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
    CHECK(strcmp(nl_langinfo_l(CODESET, global_c_snapshot), "ASCII") == 0);

    CHECK(setlocale(LC_ALL, "C.UTF-8") != NULL);
    CHECK(strcmp(nl_langinfo_l(CODESET, global_c_snapshot), "ASCII") == 0);
    errno = EAGAIN;
    global_utf8_snapshot = duplicate_locale(LC_GLOBAL_LOCALE);
    CHECK(global_utf8_snapshot != (locale_t)0);
    CHECK(errno == EAGAIN);
    CHECK(strcmp(nl_langinfo_l(CODESET, global_utf8_snapshot), "UTF-8") == 0);

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
