#include <stdio.h>
#include <stdlib.h>

__attribute__((constructor)) static void user_preload_constructor(void) {
    const char* path = getenv("MUSL_BSD_TEST_PRELOAD_TRACE");
    FILE* stream;

    if (path == NULL)
        return;
    stream = fopen(path, "a");
    if (stream != NULL) {
        fputs("user-preload-constructor\n", stream);
        fclose(stream);
    }
}
