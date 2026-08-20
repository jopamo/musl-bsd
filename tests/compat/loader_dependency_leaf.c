#include <stdio.h>
#include <stdlib.h>

static int leaf_ready;

void loader_dependency_trace(const char* event) {
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

int loader_dependency_leaf_ready(void) {
    return leaf_ready;
}

__attribute__((constructor)) static void leaf_constructor(void) {
    loader_dependency_trace("dependency-leaf-constructor");
    leaf_ready = 1;
}

__attribute__((destructor)) static void leaf_destructor(void) {
    loader_dependency_trace(leaf_ready == 1 ? "dependency-leaf-destructor" : "dependency-leaf-invalid");
    leaf_ready = 0;
}
