#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char* __strdup(const char* string);

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "strdup ABI test failed at line %d\n", __LINE__); \
            return 1;                                                         \
        }                                                                     \
    } while (0)

int main(void) {
    static const char source[] = "alpha\0ignored";
    char* (*duplicate)(const char* string) = __strdup;
    char* copy;
    char* empty;
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)duplicate, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") != NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__strdup") == (void*)duplicate);

    errno = EDOM;
    copy = duplicate(source);
    CHECK(copy != NULL);
    CHECK(errno == EDOM);
    CHECK(strcmp(copy, "alpha") == 0);
    CHECK(strlen(copy) == 5);
    CHECK(memcmp(copy, source, sizeof("alpha")) == 0);

    errno = ERANGE;
    empty = duplicate("");
    CHECK(empty != NULL);
    CHECK(empty != copy);
    CHECK(errno == ERANGE);
    CHECK(empty[0] == '\0');

    copy[0] = 'A';
    CHECK(source[0] == 'a');
    CHECK(strcmp(copy, "Alpha") == 0);
    CHECK(strcmp(empty, "") == 0);

    free(empty);
    free(copy);
    return 0;
}
