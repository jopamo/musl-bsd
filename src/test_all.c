#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include "fts.h"
#include "obstack.h"

#ifndef obstack_chunk_alloc
#define obstack_chunk_alloc malloc
#endif
#ifndef obstack_chunk_free
#define obstack_chunk_free free
#endif

static void test_fts(int argc, char* argv[]);
static void test_obstack(void);

int main(int argc, char* argv[]) {
    if (argc > 1) {
        test_fts(argc, argv);
    }
    else {
        char* fake_argv[2];
        fake_argv[0] = ".";
        fake_argv[1] = NULL;
        test_fts(1, fake_argv);
    }

    test_obstack();

    return 0;
}

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

    printf("[FTS TEST] Traversing file hierarchy...\n");
    while ((ent = fts_read(fts_handle)) != NULL) {
        printf("  [fts_read] Node at level=%d: \"%s\"\n", ent->fts_level, ent->fts_path);

        if (ent->fts_statp) {
            struct stat* st = ent->fts_statp;
            printf("    Dev/Ino: %lu/%lu, Mode: 0%o\n", (unsigned long)st->st_dev, (unsigned long)st->st_ino,
                   (unsigned)st->st_mode);
        }

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

        if (ent->fts_info == FTS_D) {
            printf("    => Attempting fts_children on this directory...\n");
            FTSENT* ch = fts_children(fts_handle, 0);
            if (!ch) {
                if (errno) {
                    fprintf(stderr, "      fts_children error: %s\n", strerror(errno));
                }
                else {
                    printf("      (No children in this directory)\n");
                }
            }
            else {
                FTSENT* c = ch;
                while (c) {
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

    if (errno != 0) {
        fprintf(stderr, "[FTS TEST] fts_read returned NULL with errno: %s\n", strerror(errno));
    }

    if (fts_close(fts_handle) < 0) {
        fprintf(stderr, "[FTS TEST] fts_close failed: %s\n", strerror(errno));
    }

    free(paths);
    printf("[FTS TEST] Done with file hierarchy.\n\n");
}

static void test_failed_allocation_handler(void) {
    fprintf(stderr, "obstack_alloc_failed_handler: Allocation failed!\n");
    exit(1);
}

static void* custom_allocator(size_t size) {
    printf("[custom_allocator] Called with size = %zu\n", size);
    return malloc(size);
}

static void custom_deallocator(void* ptr) {
    printf("[custom_deallocator] Called on ptr = %p\n", ptr);
    free(ptr);
}

static void* custom_allocator_with_arg(void* arg, size_t size) {
    printf("[custom_allocator_with_arg] arg = %p, size = %zu\n", arg, size);
    return malloc(size);
}

static void custom_deallocator_with_arg(void* arg, void* ptr) {
    printf("[custom_deallocator_with_arg] arg = %p, ptr = %p\n", arg, ptr);
    free(ptr);
}

static void test_obstack(void) {
    {
        struct obstack myobstack;
        char *str1, *str2, *str3;
        size_t total_used;

        printf("[OBSTACK TEST] Initializing obstack...\n");
        obstack_init(&myobstack);

        printf("[OBSTACK TEST] Creating first object (\"Hello, World!\")...\n");
        obstack_grow(&myobstack, "Hello, ", 7);
        obstack_grow0(&myobstack, "World!", 6);
        str1 = obstack_finish(&myobstack);
        printf("  => Built string: \"%s\" (ptr=%p)\n", str1, (void*)str1);

        printf("[OBSTACK TEST] Creating second object (\"Another string\")...\n");
        obstack_grow0(&myobstack, "Another string", 14);
        str2 = obstack_finish(&myobstack);
        printf("  => Built string: \"%s\" (ptr=%p)\n", str2, (void*)str2);

        printf("[OBSTACK TEST] Using obstack_printf (\"Number: 42 [the answer]\")...\n");
        obstack_printf(&myobstack, "%s %d %s", "Number:", 42, "[the answer]");
        str3 = obstack_finish(&myobstack);
        printf("  => Formatted string: \"%s\" (ptr=%p)\n", str3, (void*)str3);

        total_used = _obstack_memory_used(&myobstack);
        printf("[OBSTACK TEST] Currently using %zu bytes in obstack.\n", (size_t)total_used);

        printf("[OBSTACK TEST] Freeing all data in the obstack...\n");
        obstack_free(&myobstack, NULL);
        printf("  => Freed all data. Now _obstack_memory_used = %zu\n", (size_t)_obstack_memory_used(&myobstack));

        printf("[OBSTACK TEST] Done with simpler demonstration.\n\n");
    }

    {
        printf("=== [OBSTACK ADVANCED TEST] ===\n\n");

        obstack_alloc_failed_handler = test_failed_allocation_handler;

        {
            printf("=== Test 1: obstack_init / basic usage ===\n");
            struct obstack adv_obstack;
            obstack_init(&adv_obstack);

            printf("Initially, obstack_empty_p? %d\n", obstack_empty_p(&adv_obstack));

            obstack_1grow(&adv_obstack, 'A');
            obstack_1grow(&adv_obstack, 'B');
            obstack_1grow(&adv_obstack, 'C');

            printf("Object size after 3 chars: %zu\n", obstack_object_size(&adv_obstack));
            printf("Room left in current chunk: %zu\n", obstack_room(&adv_obstack));
            printf("Memory used (so far): %zu\n", obstack_memory_used(&adv_obstack));

            const char* testStr = "Hello, obstack!";
            obstack_grow(&adv_obstack, testStr, strlen(testStr));
            obstack_1grow(&adv_obstack, '\0');

            char* myString = obstack_finish(&adv_obstack);
            printf("Finished object/string: \"%s\"\n", myString);
            printf("Now obstack_empty_p? %d\n", obstack_empty_p(&adv_obstack));

            obstack_free(&adv_obstack, myString);
            printf("After free, obstack_empty_p? %d\n\n", obstack_empty_p(&adv_obstack));

            printf("=== Test 2: obstack_printf ===\n");
            obstack_printf(&adv_obstack, "This is a test: %d + %d = %d\n", 2, 3, 5);
            char* fmtString = obstack_finish(&adv_obstack);
            printf("Result from obstack_printf:\n%s", fmtString);

            obstack_free(&adv_obstack, fmtString);
            printf("After freeing printf-string, empty? %d\n\n", obstack_empty_p(&adv_obstack));

            printf("=== Test 3: obstack_int_grow ===\n");
            obstack_int_grow(&adv_obstack, 42);
            obstack_int_grow(&adv_obstack, 999);

            int* intArray = obstack_finish(&adv_obstack);
            printf("Two ints stored: [%d, %d]\n", intArray[0], intArray[1]);

            obstack_free(&adv_obstack, intArray);
            printf("After freeing int array, empty? %d\n\n", obstack_empty_p(&adv_obstack));

            printf("=== Test 4: obstack_blank / obstack_make_room ===\n");
            obstack_blank(&adv_obstack, 20);
            printf("Object size after obstack_blank(20): %zu\n", obstack_object_size(&adv_obstack));
            obstack_make_room(&adv_obstack, 50);
            printf("Room after obstack_make_room(50): %zu\n", obstack_room(&adv_obstack));

            void* blankObject = obstack_finish(&adv_obstack);
            printf("Blank object pointer = %p\n", blankObject);

            obstack_free(&adv_obstack, blankObject);
            printf("After freeing blank object, empty? %d\n\n", obstack_empty_p(&adv_obstack));

            printf("=== Test 5: obstack_specify_allocation ===\n");
            struct obstack customObstack;
            obstack_specify_allocation(&customObstack, 0, 0, custom_allocator, custom_deallocator);
            obstack_1grow(&customObstack, 'X');
            obstack_1grow(&customObstack, 'Y');
            obstack_1grow(&customObstack, 'Z');
            char* xyz = obstack_finish(&customObstack);
            printf("Custom-obstack string = \"%s\"\n", xyz);

            obstack_free(&customObstack, xyz);
            obstack_free(&customObstack, 0);
            printf("Freed entire custom obstack region.\n\n");

            printf("=== Test 6: obstack_specify_allocation_with_arg ===\n");
            struct obstack customArgObstack;
            int extraValue = 1234;
            obstack_specify_allocation_with_arg(&customArgObstack, 0, 0, custom_allocator_with_arg,
                                                custom_deallocator_with_arg, &extraValue);

            obstack_grow(&customArgObstack, "CustomArg", 9);
            obstack_1grow(&customArgObstack, '\0');
            char* customArgStr = obstack_finish(&customArgObstack);
            printf("String in customArgObstack: \"%s\"\n", customArgStr);

            obstack_free(&customArgObstack, customArgStr);
            obstack_free(&customArgObstack, 0);
            printf("Freed entire customArgObstack region.\n\n");

            printf("=== Test 7: direct usage of _obstack_begin_1 etc. ===\n");
            struct obstack directObstack;
            _obstack_begin_1(&directObstack, 64, 16, custom_allocator_with_arg, custom_deallocator_with_arg,
                             &extraValue);

            printf("Calling _obstack_newchunk(&directObstack, 128)\n");
            _obstack_newchunk(&directObstack, 128);

            memset(directObstack.next_free, '@', 10);
            directObstack.next_free += 10;
            *directObstack.next_free++ = '\0';

            char* directStr = obstack_finish(&directObstack);
            printf("String from direct usage: \"%s\"\n", directStr);

            printf("Calling _obstack_free(&directObstack, directStr)...\n");
            _obstack_free(&directObstack, directStr);

            obstack_free(&directObstack, 0);
            printf("=== Test 7 done. ===\n\n");

            printf("All advanced tests done! Cleaning up adv_obstack.\n");
            obstack_free(&adv_obstack, 0);
            printf("=== [OBSTACK ADVANCED TEST] COMPLETE ===\n\n");
        }
    }
}
