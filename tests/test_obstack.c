#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <obstack.h>

void print_hex(const char* str) {
    printf("Hexadecimal representation: ");
    while (*str) {
        printf("%02x ", (unsigned char)(*str));
        str++;
    }
    printf("\n");
}

static int test_obstack(void) {
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
    if (strcmp(str1, "Hello, World!") != 0) {
        printf("[ERROR] First object mismatch: Expected \"Hello, World!\", got \"%s\"\n", str1);
        return 1;
    }

    printf("[OBSTACK TEST] Creating second object (\"Another string\")...\n");
    obstack_grow0(&myobstack, "Another string", 14);
    str2 = obstack_finish(&myobstack);
    printf("  => Built string: \"%s\" (ptr=%p)\n", str2, (void*)str2);
    if (strcmp(str2, "Another string") != 0) {
        printf("[ERROR] Second object mismatch: Expected \"Another string\", got \"%s\"\n", str2);
        return 1;
    }

    printf("[OBSTACK TEST] Using obstack_printf (\"Number: 42 [the answer]\")...\n");

    obstack_free(&myobstack, NULL);

    obstack_printf(&myobstack, "%s %d %s", "Number:", 42, "[the answer]");

    str3 = obstack_finish(&myobstack);

    printf("[DEBUG] Raw formatted string before finishing: \"%s\"\n", str3);
    printf("  => Formatted string: \"%s\" (ptr=%p)\n", str3, (void*)str3);

    print_hex(str3);

    if (strcmp(str3, "Number: 42 [the answer]") != 0) {
        printf("[ERROR] Formatted string mismatch: Expected \"Number: 42 [the answer]\", got \"%s\"\n", str3);
        return 1;
    }

    total_used = _obstack_memory_used(&myobstack);
    printf("[OBSTACK TEST] Currently using %zu bytes in obstack.\n", (size_t)total_used);

    printf("[OBSTACK TEST] Freeing all data in the obstack...\n");
    obstack_free(&myobstack, NULL);
    total_used = _obstack_memory_used(&myobstack);
    printf("  => Freed all data. Now _obstack_memory_used = %zu\n", (size_t)total_used);

    printf("[OBSTACK TEST] Done with the tests.\n\n");

    return 0;
}

int main(void) {
    int result = test_obstack();
    if (result == 0) {
        printf("All tests passed!\n");
    }
    else {
        printf("Some tests failed.\n");
    }
    return result;
}
