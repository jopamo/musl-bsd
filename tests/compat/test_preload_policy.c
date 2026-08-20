#include "../../src/compat/preload_policy.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                           \
    do {                                                                           \
        if (!(condition)) {                                                        \
            fprintf(stderr, "preload policy check failed at line %d\n", __LINE__); \
            return 1;                                                              \
        }                                                                          \
    } while (0)

struct preload_case {
    const char* core;
    const char* nvidia_tls;
    const char* user;
    const char* expected;
};

int main(void) {
    static const struct preload_case cases[] = {
        {"/core.so", NULL, NULL, "/core.so"},
        {"/core.so", "", "", "/core.so"},
        {"/core.so", "/nvidia-tls.so", NULL, "/core.so:/nvidia-tls.so"},
        {"/core.so", NULL, "/user-a.so:/user-b.so", "/core.so:/user-a.so:/user-b.so"},
        {"/core.so", "/nvidia-tls.so", "/user-a.so:/user-b.so", "/core.so:/nvidia-tls.so:/user-a.so:/user-b.so"},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        char* list = musl_bsd_preload_list(cases[i].core, cases[i].nvidia_tls, cases[i].user);
        CHECK(list != NULL);
        CHECK(strcmp(list, cases[i].expected) == 0);
        free(list);
    }

    errno = 0;
    CHECK(musl_bsd_preload_list(NULL, NULL, NULL) == NULL);
    CHECK(errno == EINVAL);
    errno = 0;
    CHECK(musl_bsd_preload_list("", NULL, NULL) == NULL);
    CHECK(errno == EINVAL);
    errno = 0;
    CHECK(musl_bsd_preload_list("/core.so", "relative-tls.so", NULL) == NULL);
    CHECK(errno == EINVAL);
    errno = 0;
    CHECK(musl_bsd_preload_list("/core.so", "/tls.so:/other.so", NULL) == NULL);
    CHECK(errno == EINVAL);

    CHECK(unsetenv("MUSL_BSD_TEST_COMPATIBILITY_PATH") == 0);
    CHECK(strcmp(musl_bsd_compatibility_path("MUSL_BSD_TEST_COMPATIBILITY_PATH", "/configured"), "/configured") == 0);
    CHECK(setenv("MUSL_BSD_TEST_COMPATIBILITY_PATH", "/environment", 1) == 0);
    CHECK(strcmp(musl_bsd_compatibility_path("MUSL_BSD_TEST_COMPATIBILITY_PATH", "/configured"), "/environment") == 0);
    CHECK(setenv("MUSL_BSD_TEST_COMPATIBILITY_PATH", "", 1) == 0);
    CHECK(strcmp(musl_bsd_compatibility_path("MUSL_BSD_TEST_COMPATIBILITY_PATH", "/configured"), "/configured") == 0);
    CHECK(unsetenv("MUSL_BSD_TEST_COMPATIBILITY_PATH") == 0);
    return 0;
}
