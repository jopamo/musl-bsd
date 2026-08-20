#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

#ifdef ftruncate64
#undef ftruncate64
#endif

#ifdef statfs64
#undef statfs64
#endif

extern int ftruncate64(int fd, off64_t length);
extern int statfs64(const char* path, struct statfs* buf);

typedef int (*ftruncate64_function)(int fd, off64_t length);
typedef int (*statfs64_function)(const char* path, struct statfs* buf);

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "fs64 ABI test failed at line %d\n", __LINE__); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

static int verify_core_provider(const char* name, const void* function) {
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr(function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") != NULL);
    CHECK(dlsym(RTLD_DEFAULT, name) == function);
    return 0;
}

int main(void) {
    static const off64_t large_size = ((off64_t)UINT64_C(1) << 32) + 123;
    char path[] = "/tmp/musl-bsd-fs64-XXXXXX";
    char missing_path[] = "/tmp/musl-bsd-fs64-missing";
    ftruncate64_function truncate_function = ftruncate64;
    statfs64_function statfs_function = statfs64;
    struct stat status;
    struct statfs filesystem;
    int fd;

    CHECK(verify_core_provider("ftruncate64", (const void*)truncate_function) == 0);
    CHECK(verify_core_provider("statfs64", (const void*)statfs_function) == 0);
#if defined(__x86_64__) && __SIZEOF_POINTER__ == 8
    CHECK(sizeof(off64_t) == sizeof(off_t));
#endif

    fd = mkstemp(path);
    CHECK(fd >= 0);
    CHECK(unlink(path) == 0);
    errno = EDOM;
    CHECK(truncate_function(fd, large_size) == 0);
    CHECK(errno == EDOM);
    CHECK(fstat(fd, &status) == 0);
    CHECK(status.st_size == large_size);
    errno = 0;
    CHECK(truncate_function(-1, large_size) == -1);
    CHECK(errno == EBADF);
    errno = 0;
    CHECK(truncate_function(fd, (off64_t)-1) == -1);
    CHECK(errno == EINVAL);
    CHECK(close(fd) == 0);

    errno = ERANGE;
    CHECK(statfs_function(".", &filesystem) == 0);
    CHECK(errno == ERANGE);
    CHECK(filesystem.f_type != 0);
    CHECK(filesystem.f_bsize > 0);
    errno = 0;
    CHECK(statfs_function(missing_path, &filesystem) == -1);
    CHECK(errno == ENOENT);
    return 0;
}
