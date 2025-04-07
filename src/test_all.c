/******************************************************************************
 * test_all.c (expanded debugging)
 *
 * A demonstration program testing:
 *   1) The fts(3)-like API (fts_open, fts_read, fts_children, fts_set, fts_close)
 *      with extra debug prints.
 *   2) The obstack API (_obstack_begin, obstack_* macros, etc.)
 *      also with extra debug prints.
 *
 * Example build:
 *   cc -o test_all test_all.c fts.c obstack.c \
 *       -I./include -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>     /* for printing timestamps if you like */
#include <sys/stat.h> /* if you want to show st_mode, etc. */

#include "fts.h"
#include "obstack.h"

/* If you want to use standard malloc/free for obstack, define these macros: */
#ifndef obstack_chunk_alloc
#define obstack_chunk_alloc malloc
#endif
#ifndef obstack_chunk_free
#define obstack_chunk_free free
#endif

/* Forward declarations for test functions */
static void test_fts(int argc, char* argv[]);
static void test_obstack(void);

int main(int argc, char* argv[]) {
    /* If you run with arguments, we'll treat them as paths for the FTS test.
     * Otherwise, we default to "." for demonstration. */
    if (argc > 1) {
        test_fts(argc, argv);
    }
    else {
        char* fake_argv[2];
        fake_argv[0] = ".";
        fake_argv[1] = NULL;
        test_fts(1, fake_argv);
    }

    /* Also demonstrate an obstack usage so you can see how it works. */
    test_obstack();

    return 0;
}

/******************************************************************************
 * test_fts
 *  Demonstrate usage of fts_open, fts_read, fts_children, fts_set, fts_close
 *  with more verbose debug prints.
 *****************************************************************************/
static void test_fts(int argc, char* argv[]) {
    FTS* fts_handle;
    FTSENT* ent;
    int fts_options = FTS_NOCHDIR | FTS_PHYSICAL; /* example options */

    char** paths = NULL;
    int i;
    int offset = 0;

    /* We skip argv[0] if multiple arguments are provided. */
    if (argc > 1) {
        offset = 1;
    }
    paths = calloc((argc - offset) + 1, sizeof(char*));
    if (!paths) {
        fprintf(stderr, "Out of memory building path array!\n");
        return;
    }

    /* Fill the 'paths' array with the user-supplied arguments (or "." by default). */
    for (i = offset; i < argc; i++) {
        paths[i - offset] = argv[i];
    }
    paths[argc - offset] = NULL; /* Null terminator */

    printf("\n[FTS TEST] Attempting fts_open with the following path list:\n");
    for (i = 0; paths[i] != NULL; i++) {
        printf("  paths[%d] = \"%s\"\n", i, paths[i]);
    }

    /* Open the file hierarchy for traversal. */
    fts_handle = fts_open(paths, fts_options, NULL);
    if (!fts_handle) {
        fprintf(stderr, "fts_open failed: %s\n", strerror(errno));
        free(paths);
        return;
    }
    printf("[FTS TEST] fts_open succeeded, pointer = %p\n", (void*)fts_handle);

    /* Read entries in a loop. */
    printf("[FTS TEST] Traversing file hierarchy...\n");
    while ((ent = fts_read(fts_handle)) != NULL) {
        /*
         * ent->fts_info gives us the type of the current node,
         * ent->fts_path is the full path string,
         * ent->fts_level is the depth in the tree,
         * ent->fts_errno if there's an error, etc.
         */
        printf("  [fts_read] Node at level=%d: \"%s\"\n", ent->fts_level, ent->fts_path);

        /* If we have stat info stored, we could show st_mode, etc.
         * That depends on fts usage. E.g.:
         */
        if (ent->fts_statp) {
            struct stat* st = ent->fts_statp;
            printf("    Dev/Ino: %lu/%lu, Mode: 0%o\n", (unsigned long)st->st_dev, (unsigned long)st->st_ino,
                   (unsigned)st->st_mode);
        }

        /* Show the fts_info classification in a more descriptive manner. */
        switch (ent->fts_info) {
            case FTS_D:
                printf("    => Directory (pre-order)\n");
                break;
            case FTS_DP:
                printf("    => Directory (post-order)\n");
                break;
            case FTS_F:
                printf("    => Regular file\n");
                break;
            case FTS_SL:
                printf("    => Symlink\n");
                break;
            case FTS_ERR:
                printf("    => ERROR (errno=%d)\n", ent->fts_errno);
                break;
            case FTS_DNR:
                printf("    => Directory not readable (errno=%d)\n", ent->fts_errno);
                break;
            case FTS_NS:
                printf("    => No stat info available (errno=%d)\n", ent->fts_errno);
                break;
            case FTS_DC:
                printf("    => Directory cycle!\n");
                break;
            case FTS_DOT:
                printf("    => Dot directory (\".\" or \"..\")\n");
                break;
            default:
                printf("    => Other node type (fts_info=%d)\n", ent->fts_info);
                break;
        }

        /*
         * Optionally demonstrate fts_children to see the subitems of a directory
         * before continuing the normal read. Usually you might not need this.
         */
        if (ent->fts_info == FTS_D) {
            printf("    => Attempting fts_children on this directory...\n");
            FTSENT* ch = fts_children(fts_handle, 0);
            if (!ch) {
                /* Either no children or an error. Check errno. */
                if (errno) {
                    fprintf(stderr, "      fts_children error: %s\n", strerror(errno));
                }
                else {
                    printf("      (No children in this directory)\n");
                }
            }
            else {
                /* We can iterate the returned linked list of children. */
                FTSENT* c = ch;
                while (c) {
                    /* Example: if it's a dot dir, skip it. */
                    if (c->fts_info == FTS_DOT) {
                        printf("      Child: \"%s\" is DOT, skipping.\n", c->fts_name);
                        fts_set(fts_handle, c, FTS_SKIP);
                    }
                    else {
                        printf("      Child: \"%s\"\n", c->fts_name);
                    }
                    c = c->fts_link;
                }
            }
        }
    }

    /* If the loop ended because of an error, errno will be nonzero. */
    if (errno != 0) {
        fprintf(stderr, "[FTS TEST] fts_read returned NULL with errno: %s\n", strerror(errno));
    }

    /* Close the FTS handle. */
    if (fts_close(fts_handle) < 0) {
        fprintf(stderr, "[FTS TEST] fts_close failed: %s\n", strerror(errno));
    }

    free(paths);
    printf("[FTS TEST] Done with file hierarchy.\n\n");
}

/******************************************************************************
 * test_obstack
 *  Demonstrate usage of obstack: building string objects, etc. with extra info.
 *****************************************************************************/
static void test_obstack(void) {
    struct obstack myobstack;
    char *str1, *str2, *str3;
    size_t total_used;

    /* Initialize with a default chunk alloc & free. */
    printf("[OBSTACK TEST] Initializing obstack...\n");
    obstack_init(&myobstack);

    /* Build a small string object. */
    printf("[OBSTACK TEST] Creating first object (\"Hello, World!\")...\n");
    obstack_grow(&myobstack, "Hello, ", 7);
    obstack_grow0(&myobstack, "World!", 6);
    str1 = obstack_finish(&myobstack);
    printf("  => Built string: \"%s\" (ptr=%p)\n", str1, (void*)str1);

    /* Build another string object quickly. */
    printf("[OBSTACK TEST] Creating second object (\"Another string\")...\n");
    obstack_grow0(&myobstack, "Another string", 14);
    str2 = obstack_finish(&myobstack);
    printf("  => Built string: \"%s\" (ptr=%p)\n", str2, (void*)str2);

    /* We can also format strings into an obstack. */
    printf("[OBSTACK TEST] Using obstack_printf (\"Number: 42 [the answer]\")...\n");
    obstack_printf(&myobstack, "%s %d %s", "Number:", 42, "[the answer]");
    str3 = obstack_finish(&myobstack);
    printf("  => Formatted string: \"%s\" (ptr=%p)\n", str3, (void*)str3);

    /* Show how many bytes the obstack has allocated in total. */
    total_used = _obstack_memory_used(&myobstack);
    printf("[OBSTACK TEST] Currently using %zu bytes in obstack.\n", (size_t)total_used);

    /* Example: free everything in the obstack with obstack_free(..., NULL). */
    printf("[OBSTACK TEST] Freeing all data in the obstack...\n");
    obstack_free(&myobstack, NULL);
    printf("  => Freed all data. Now _obstack_memory_used = %zu\n", (size_t)_obstack_memory_used(&myobstack));

    /* The obstack can still be used further if desired. */
    printf("[OBSTACK TEST] Done.\n\n");
}
