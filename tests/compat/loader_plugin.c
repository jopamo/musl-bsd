#include <stdio.h>
#include <stdlib.h>

static void trace(const char* event) {
    const char* path = getenv("MUSL_BSD_TEST_TRACE");
    FILE* stream;

    if (path == NULL)
        return;
    stream = fopen(path, "a");
    if (stream != NULL) {
        fprintf(stream, "%s\n", event);
        fclose(stream);
    }
}

__attribute__((constructor)) static void plugin_constructor(void) {
    trace("plugin-constructor");
}

__attribute__((destructor)) static void plugin_destructor(void) {
    trace("plugin-destructor");
}

int loader_plugin_entry(void) {
    trace("plugin-entry");
    return 73;
}
