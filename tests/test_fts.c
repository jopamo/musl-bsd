#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fts.h>

static void test_fts(int argc, char* argv[]) {
    FTS* fts_handle;
    FTSENT* ent;
    int fts_options = FTS_NOCHDIR | FTS_PHYSICAL;

    char** paths = NULL;
    int i;
    int offset = 0;

    if (argc > 1) {
        offset = 1;
    }

    paths = calloc((size_t)(argc - offset) + 1, sizeof(char*));
    if (!paths) {
        fprintf(stderr, "Out of memory building path array!\n");
        return;
    }

    for (i = offset; i < argc; i++) {
        paths[i - offset] = argv[i];
    }
    paths[argc - offset] = NULL;

    printf("\n[FTS TEST] Attempting fts_open with the following path list:\n");
    for (i = 0; paths[i] != NULL; i++) {
        printf("  paths[%d] = \"%s\"\n", i, paths[i]);
    }

    fts_handle = fts_open(paths, fts_options, NULL);
    if (!fts_handle) {
        fprintf(stderr, "fts_open failed: %s\n", strerror(errno));
        free(paths);
        return;
    }

    printf("[FTS TEST] fts_open succeeded, pointer = %p\n", (void*)fts_handle);

    // Traverse the file hierarchy
    while ((ent = fts_read(fts_handle)) != NULL) {
        printf("  [fts_read] Node at level=%d: \"%s\"\n", ent->fts_level, ent->fts_path);
    }

    if (fts_close(fts_handle) < 0) {
        fprintf(stderr, "[FTS TEST] fts_close failed: %s\n", strerror(errno));
    }

    free(paths);
    printf("[FTS TEST] Done with file hierarchy.\n\n");
}

int main(int argc, char* argv[]) {
    test_fts(argc, argv);
    return 0;
}
