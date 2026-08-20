#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

extern unsigned long musl_bsd_core_abi(void);

#define CHECK(condition)  \
    do {                  \
        if (!(condition)) \
            return 1;     \
    } while (0)

int main(void) {
    static const char* const hook_names[] = {
        "__malloc_hook",
        "__realloc_hook",
        "__free_hook",
        "__memalign_hook",
    };
    void* allocation;

    CHECK(musl_bsd_core_abi() == (unsigned long)UINT64_C(0x4d42534400020000));
    for (size_t index = 0; index < sizeof(hook_names) / sizeof(hook_names[0]); ++index) {
        const char* error;

        dlerror();
        errno = EDOM;
        CHECK(dlsym(RTLD_DEFAULT, hook_names[index]) == NULL);
        error = dlerror();
        CHECK(error != NULL);
        CHECK(errno == EDOM);
        CHECK(dlerror() == NULL);
    }

    allocation = malloc(32);
    CHECK(allocation != NULL);
    allocation = realloc(allocation, 64);
    CHECK(allocation != NULL);
    free(allocation);
    return 0;
}
