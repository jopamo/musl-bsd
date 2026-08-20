#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

extern void* __rawmemchr(const void* ptr, int byte);

#define CHECK(condition)  \
    do {                  \
        if (!(condition)) \
            return 1;     \
    } while (0)

static int test_first_match_and_byte_conversion(void) {
    unsigned char bytes[64];

    memset(bytes, 0x31, sizeof(bytes));
    bytes[2] = 0x7e;
    bytes[17] = 0x7e;
    CHECK(__rawmemchr(bytes, 0x7e) == bytes + 2);
    CHECK(__rawmemchr(bytes + 3, 0x7e) == bytes + 17);

    bytes[23] = 0xff;
    CHECK(__rawmemchr(bytes, -1) == bytes + 23);
    CHECK(__rawmemchr(bytes, 0x1ff) == bytes + 23);

    bytes[41] = 0;
    CHECK(__rawmemchr(bytes, 0) == bytes + 41);
    return 0;
}

static int test_unaligned_starts(void) {
    unsigned char bytes[96];

    for (size_t offset = 0; offset < 32; ++offset) {
        memset(bytes, 0xa5, sizeof(bytes));
        bytes[offset + 47] = 0x5c;
        CHECK(__rawmemchr(bytes + offset, 0x5c) == bytes + offset + 47);
    }
    return 0;
}

static int test_guarded_page_boundary(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    unsigned char* mapping;
    unsigned char* target;

    CHECK(page_size > 128);
    mapping = mmap(NULL, (size_t)page_size * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(mapping != MAP_FAILED);
    if (mprotect(mapping + page_size, (size_t)page_size, PROT_NONE) != 0) {
        munmap(mapping, (size_t)page_size * 2);
        return 1;
    }

    target = mapping + page_size - 1;
    memset(target - 127, 0x44, 127);
    *target = 0x9b;
    errno = EDOM;
    if (__rawmemchr(target - 127, 0x9b) != target || errno != EDOM) {
        munmap(mapping, (size_t)page_size * 2);
        return 1;
    }
    CHECK(munmap(mapping, (size_t)page_size * 2) == 0);
    return 0;
}

int main(void) {
    CHECK(test_first_match_and_byte_conversion() == 0);
    CHECK(test_unaligned_starts() == 0);
    CHECK(test_guarded_page_boundary() == 0);
    return 0;
}
