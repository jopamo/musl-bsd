#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*asprintf_function)(char** output, const char* format, ...);

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "asprintf ABI test failed at line %d\n", __LINE__); \
            return 1;                                                           \
        }                                                                       \
    } while (0)

static int verify_formats(asprintf_function function) {
#define CHECK_ASPRINTF(condition)                                               \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "asprintf ABI test failed at line %d\n", __LINE__); \
            goto cleanup;                                                       \
        }                                                                       \
    } while (0)
    static const char embedded_expected[] = {'a', '\0', 't', 'a', 'i', 'l', '\0'};
    char* output = NULL;
    int length;
    int result = 1;

    errno = EDOM;
    length = function(&output, "gpu-%d-%s", 42, "ok");
    CHECK_ASPRINTF(length == 9);
    CHECK_ASPRINTF(output != NULL);
    CHECK_ASPRINTF(strcmp(output, "gpu-42-ok") == 0);
    CHECK_ASPRINTF(errno == EDOM);
    free(output);
    output = NULL;

    errno = ERANGE;
    length = function(&output, "a%c%s", '\0', "tail");
    CHECK_ASPRINTF(length == 6);
    CHECK_ASPRINTF(output != NULL);
    CHECK_ASPRINTF(memcmp(output, embedded_expected, sizeof(embedded_expected)) == 0);
    CHECK_ASPRINTF(errno == ERANGE);
    free(output);
    output = NULL;

    errno = E2BIG;
    length = function(&output, "%s", "");
    CHECK_ASPRINTF(length == 0);
    CHECK_ASPRINTF(output != NULL);
    CHECK_ASPRINTF(output[0] == '\0');
    CHECK_ASPRINTF(errno == E2BIG);
    result = 0;

cleanup:
    free(output);
#undef CHECK_ASPRINTF
    return result;
}

int main(void) {
    asprintf_function function = asprintf;
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "asprintf") == (void*)function);
    CHECK(verify_formats(function) == 0);
    return 0;
}
