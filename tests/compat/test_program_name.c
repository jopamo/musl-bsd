#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern char* __progname_full;

#define CHECK(condition)                                                            \
    do {                                                                            \
        if (!(condition)) {                                                         \
            fprintf(stderr, "program-name ABI test failed at line %d\n", __LINE__); \
            return 1;                                                               \
        }                                                                           \
    } while (0)

int main(int argc, char** argv) {
    char** full_symbol;
    char** invocation_symbol;
    const char* basename;
    /* Keep the alias comparison at runtime; the two extern declarations may
     * be folded as distinct objects by the compiler. */
    volatile uintptr_t full_address;
    volatile uintptr_t invocation_address;
    Dl_info info;

    CHECK(argc > 0);
    CHECK(argv != NULL);
    CHECK(argv[0] != NULL);

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)&__progname_full, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);

    full_symbol = dlsym(RTLD_DEFAULT, "__progname_full");
    invocation_symbol = dlsym(RTLD_DEFAULT, "program_invocation_name");
    CHECK(full_symbol == &__progname_full);
    CHECK(invocation_symbol == &program_invocation_name);
    full_address = (uintptr_t)full_symbol;
    invocation_address = (uintptr_t)invocation_symbol;
    CHECK(full_address == invocation_address);
    CHECK(__progname_full == argv[0]);
    CHECK(strcmp(__progname_full, argv[0]) == 0);

    basename = strrchr(argv[0], '/');
    if (basename != NULL)
        ++basename;
    else
        basename = argv[0];
    CHECK(program_invocation_short_name != NULL);
    CHECK(strcmp(program_invocation_short_name, basename) == 0);

    errno = EDOM;
    CHECK(__progname_full == argv[0]);
    CHECK(errno == EDOM);
    return 0;
}
