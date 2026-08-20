#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef fallocate64
#undef fallocate64
#endif

extern int fallocate64(int fd, int mode, off64_t offset, off64_t length);

#if defined(__x86_64__) && __SIZEOF_POINTER__ == 8
typedef char glibc_off64_size[(sizeof(off64_t) == 8) ? 1 : -1];
typedef char fallocate_offset_abi[(sizeof(off64_t) == sizeof(off_t)) ? 1 : -1];
typedef char fallocate_offset_alignment[(_Alignof(off64_t) == _Alignof(off_t)) ? 1 : -1];
#endif

#define CHECK(condition)  \
    do {                  \
        if (!(condition)) \
            return 1;     \
    } while (0)

enum {
    BLOCK_SIZE = 4096,
    BLOCK_COUNT = 3,
};

static int all_bytes_equal(const unsigned char* bytes, size_t length, unsigned char expected) {
    for (size_t index = 0; index < length; ++index) {
        if (bytes[index] != expected)
            return 0;
    }
    return 1;
}

int main(void) {
    char path[] = "/tmp/musl-bsd-fallocate64-XXXXXX";
    const off64_t large_offset = (off64_t)UINT64_C(1) << 32;
    unsigned char blocks[BLOCK_COUNT * BLOCK_SIZE];
    struct stat status;
    int fd;

    fd = mkstemp(path);
    CHECK(fd >= 0);
    CHECK(unlink(path) == 0);

    errno = EDOM;
    CHECK(fallocate64(fd, 0, large_offset, BLOCK_SIZE) == 0);
    CHECK(errno == EDOM);
    CHECK(fstat(fd, &status) == 0);
    CHECK(status.st_size == large_offset + BLOCK_SIZE);

    memset(blocks, 0xa7, sizeof(blocks));
    CHECK(pwrite(fd, blocks, sizeof(blocks), 0) == (ssize_t)sizeof(blocks));
    errno = ERANGE;
    CHECK(fallocate64(fd, FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE, BLOCK_SIZE, BLOCK_SIZE) == 0);
    CHECK(errno == ERANGE);
    CHECK(fstat(fd, &status) == 0);
    CHECK(status.st_size == large_offset + BLOCK_SIZE);

    memset(blocks, 0xff, sizeof(blocks));
    CHECK(pread(fd, blocks, sizeof(blocks), 0) == (ssize_t)sizeof(blocks));
    CHECK(all_bytes_equal(blocks, BLOCK_SIZE, 0xa7));
    CHECK(all_bytes_equal(blocks + BLOCK_SIZE, BLOCK_SIZE, 0));
    CHECK(all_bytes_equal(blocks + 2 * BLOCK_SIZE, BLOCK_SIZE, 0xa7));

    errno = 0;
    CHECK(fallocate64(-1, 0, 0, BLOCK_SIZE) == -1);
    CHECK(errno == EBADF);
    errno = 0;
    CHECK(fallocate64(fd, 0, -1, BLOCK_SIZE) == -1);
    CHECK(errno == EINVAL);

    CHECK(close(fd) == 0);
    return 0;
}
