#include <dlfcn.h>
#include <errno.h>
#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define CHECK(condition)                                                                  \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            fprintf(stderr, "__ctype_get_mb_cur_max test failed at line %d\n", __LINE__); \
            return 1;                                                                     \
        }                                                                                 \
    } while (0)

struct thread_check {
    locale_t locale;
    size_t expected;
    int failed;
};

static int verify_current_locale(size_t expected) {
    errno = EDOM;
    CHECK(__ctype_get_mb_cur_max() == expected);
    CHECK(errno == EDOM);
    CHECK(MB_CUR_MAX == expected);
    CHECK(errno == EDOM);
    return 0;
}

static void* verify_in_thread(void* argument) {
    struct thread_check* check = argument;
    locale_t previous = uselocale(check->locale);

    if (previous == (locale_t)0) {
        check->failed = 1;
        return NULL;
    }
    check->failed = verify_current_locale(check->expected);
    if (uselocale(previous) == (locale_t)0)
        check->failed = 1;
    return NULL;
}

int main(void) {
    size_t (*ctype_get_mb_cur_max)(void) = __ctype_get_mb_cur_max;
    static const char four_byte_character[] = "\xf0\x9f\x98\x80";
    struct thread_check thread_check;
    mbstate_t state;
    locale_t c_locale;
    locale_t previous;
    locale_t utf8_locale;
    pthread_t thread;
    wchar_t character;
    Dl_info info;

    CHECK(sizeof(ctype_get_mb_cur_max()) == sizeof(size_t));

    c_locale = newlocale(LC_CTYPE_MASK, "C", (locale_t)0);
    CHECK(c_locale != (locale_t)0);
    utf8_locale = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    CHECK(utf8_locale != (locale_t)0);

    previous = uselocale(c_locale);
    CHECK(previous != (locale_t)0);
    CHECK(verify_current_locale(1) == 0);

    memset(&thread_check, 0, sizeof(thread_check));
    thread_check.locale = utf8_locale;
    thread_check.expected = 4;
    CHECK(pthread_create(&thread, NULL, verify_in_thread, &thread_check) == 0);
    CHECK(pthread_join(thread, NULL) == 0);
    CHECK(!thread_check.failed);
    CHECK(verify_current_locale(1) == 0);

    CHECK(uselocale(utf8_locale) != (locale_t)0);
    CHECK(verify_current_locale(4) == 0);
    memset(&state, 0, sizeof(state));
    errno = EDOM;
    CHECK(mbrtowc(&character, four_byte_character, sizeof(four_byte_character) - 1, &state) == 4);
    CHECK(character == (wchar_t)0x1f600);
    CHECK(errno == EDOM);

    CHECK(uselocale(previous) != (locale_t)0);
    freelocale(utf8_locale);
    freelocale(c_locale);

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)ctype_get_mb_cur_max, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__ctype_get_mb_cur_max") == (void*)ctype_get_mb_cur_max);
    return 0;
}
