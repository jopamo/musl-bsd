#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern int __fxstat(int version, int fd, struct stat* status);
extern int __fxstat64(int version, int fd, struct stat* status);
extern int __fxstatat(int version, int dirfd, const char* path, struct stat* status, int flags);

typedef int (*fxstat_function)(int version, int fd, struct stat* status);
typedef int (*fxstatat_function)(int version, int dirfd, const char* path, struct stat* status, int flags);

struct stat_adapter {
    const char* name;
    fxstat_function function;
    int from_compatibility_core;
};

#if defined(__x86_64__) && __SIZEOF_POINTER__ == 8
_Static_assert(sizeof(struct stat) == 144, "glibc struct stat size");
_Static_assert(offsetof(struct stat, st_dev) == 0, "glibc st_dev offset");
_Static_assert(offsetof(struct stat, st_ino) == 8, "glibc st_ino offset");
_Static_assert(offsetof(struct stat, st_nlink) == 16, "glibc st_nlink offset");
_Static_assert(offsetof(struct stat, st_mode) == 24, "glibc st_mode offset");
_Static_assert(offsetof(struct stat, st_uid) == 28, "glibc st_uid offset");
_Static_assert(offsetof(struct stat, st_gid) == 32, "glibc st_gid offset");
_Static_assert(offsetof(struct stat, st_rdev) == 40, "glibc st_rdev offset");
_Static_assert(offsetof(struct stat, st_size) == 48, "glibc st_size offset");
_Static_assert(offsetof(struct stat, st_blksize) == 56, "glibc st_blksize offset");
_Static_assert(offsetof(struct stat, st_blocks) == 64, "glibc st_blocks offset");
_Static_assert(offsetof(struct stat, st_atim) == 72, "glibc st_atim offset");
_Static_assert(offsetof(struct stat, st_mtim) == 88, "glibc st_mtim offset");
_Static_assert(offsetof(struct stat, st_ctim) == 104, "glibc st_ctim offset");
#endif

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "stat ABI test failed at line %d\n", __LINE__); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

static int same_status(const struct stat* left, const struct stat* right) {
    if (left->st_dev != right->st_dev)
        return 0;
    if (left->st_ino != right->st_ino)
        return 0;
    if (left->st_nlink != right->st_nlink)
        return 0;
    if (left->st_mode != right->st_mode)
        return 0;
    if (left->st_uid != right->st_uid)
        return 0;
    if (left->st_gid != right->st_gid)
        return 0;
    if (left->st_rdev != right->st_rdev)
        return 0;
    if (left->st_size != right->st_size)
        return 0;
    if (left->st_blksize != right->st_blksize)
        return 0;
    if (left->st_blocks != right->st_blocks)
        return 0;
    if (left->st_atim.tv_sec != right->st_atim.tv_sec)
        return 0;
    if (left->st_atim.tv_nsec != right->st_atim.tv_nsec)
        return 0;
    if (left->st_mtim.tv_sec != right->st_mtim.tv_sec)
        return 0;
    if (left->st_mtim.tv_nsec != right->st_mtim.tv_nsec)
        return 0;
    if (left->st_ctim.tv_sec != right->st_ctim.tv_sec)
        return 0;
    if (left->st_ctim.tv_nsec != right->st_ctim.tv_nsec)
        return 0;
    return 1;
}

static int verify_provider(const char* name, const void* function, int expected_from_compatibility_core) {
    Dl_info info;
    int from_compatibility_core;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr(function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    from_compatibility_core = strstr(info.dli_fname, "libmusl-bsd-core") != NULL;
    CHECK(from_compatibility_core == expected_from_compatibility_core);
    CHECK(dlsym(RTLD_DEFAULT, name) == function);
    return 0;
}

static int verify_regular_file(const struct stat_adapter* adapter, int fd, const struct stat* expected) {
    static const struct {
        int version;
        int preserved_errno;
    } cases[] = {
        {1, EDOM},
        {0, ERANGE},
        {INT_MAX, E2BIG},
    };
    struct stat actual;
    size_t index;

    for (index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        memset(&actual, 0, sizeof(actual));
        errno = cases[index].preserved_errno;
        CHECK(adapter->function(cases[index].version, fd, &actual) == 0);
        CHECK(errno == cases[index].preserved_errno);
        CHECK(same_status(&actual, expected));
    }
    CHECK(S_ISREG(expected->st_mode));
    CHECK((expected->st_mode & 0777) == 0640);
    return 0;
}

static int verify_fxstatat_success(fxstatat_function function,
                                   int version,
                                   int dirfd,
                                   const char* path,
                                   int flags,
                                   const struct stat* expected,
                                   int preserved_errno) {
    struct stat actual;

    memset(&actual, 0, sizeof(actual));
    errno = preserved_errno;
    CHECK(function(version, dirfd, path, &actual, flags) == 0);
    CHECK(errno == preserved_errno);
    CHECK(same_status(&actual, expected));
    return 0;
}

static int verify_fxstatat_paths(void) {
#define CHECK_PATH(condition)                                               \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "stat ABI test failed at line %d\n", __LINE__); \
            goto cleanup;                                                   \
        }                                                                   \
    } while (0)
    static const int versions[] = {1, 0, INT_MAX};
    char directory_path[] = "/tmp/musl-bsd-fxstatat-XXXXXX";
    char target_path[PATH_MAX];
    struct stat expected_link;
    struct stat expected_target;
    struct stat status;
    int directory_created = 0;
    int directory_fd = -1;
    int file_fd = -1;
    int length;
    int result = 1;
    size_t index;

    CHECK_PATH(mkdtemp(directory_path) != NULL);
    directory_created = 1;
    directory_fd = open(directory_path, O_RDONLY | O_DIRECTORY);
    CHECK_PATH(directory_fd >= 0);
    file_fd = openat(directory_fd, "target", O_CREAT | O_EXCL | O_RDWR, 0640);
    CHECK_PATH(file_fd >= 0);
    CHECK_PATH(fchmod(file_fd, 0640) == 0);
    CHECK_PATH(fstat(file_fd, &expected_target) == 0);
    CHECK_PATH(symlinkat("target", directory_fd, "link") == 0);
    CHECK_PATH(fstatat(directory_fd, "link", &expected_link, AT_SYMLINK_NOFOLLOW) == 0);
    CHECK_PATH(S_ISLNK(expected_link.st_mode));

    for (index = 0; index < sizeof(versions) / sizeof(versions[0]); ++index) {
        CHECK_PATH(verify_fxstatat_success(__fxstatat, versions[index], directory_fd, "target", 0, &expected_target,
                                           EDOM + (int)index) == 0);
    }

    CHECK_PATH(
        verify_fxstatat_success(__fxstatat, 1, directory_fd, "link", AT_SYMLINK_NOFOLLOW, &expected_link, ENOSPC) == 0);
    CHECK_PATH(verify_fxstatat_success(__fxstatat, 1, directory_fd, "link", 0, &expected_target, ENOTTY) == 0);
    CHECK_PATH(verify_fxstatat_success(__fxstatat, 1, file_fd, "", AT_EMPTY_PATH, &expected_target, EAGAIN) == 0);

    length = snprintf(target_path, sizeof(target_path), "%s/target", directory_path);
    CHECK_PATH(length > 0);
    CHECK_PATH((size_t)length < sizeof(target_path));
    CHECK_PATH(verify_fxstatat_success(__fxstatat, 1, -1, target_path, 0, &expected_target, EBUSY) == 0);

    errno = 0;
    CHECK_PATH(__fxstatat(1, directory_fd, "missing", &status, 0) == -1);
    CHECK_PATH(errno == ENOENT);
    errno = 0;
    CHECK_PATH(__fxstatat(1, -1, "target", &status, 0) == -1);
    CHECK_PATH(errno == EBADF);
    errno = 0;
    CHECK_PATH(__fxstatat(1, directory_fd, "target", &status, 0x40000000) == -1);
    CHECK_PATH(errno == EINVAL);

    CHECK_PATH(close(file_fd) == 0);
    file_fd = -1;
    CHECK_PATH(unlinkat(directory_fd, "link", 0) == 0);
    CHECK_PATH(unlinkat(directory_fd, "target", 0) == 0);
    CHECK_PATH(close(directory_fd) == 0);
    directory_fd = -1;
    CHECK_PATH(rmdir(directory_path) == 0);
    directory_created = 0;
    result = 0;

cleanup:
    if (file_fd >= 0)
        close(file_fd);
    if (directory_fd >= 0) {
        unlinkat(directory_fd, "link", 0);
        unlinkat(directory_fd, "target", 0);
        close(directory_fd);
    }
    if (directory_created)
        rmdir(directory_path);
#undef CHECK_PATH
    return result;
}

int main(void) {
    static const struct stat_adapter adapters[] = {
        {"__fxstat", __fxstat, 0},
        {"__fxstat64", __fxstat64, 1},
    };
    static const off_t large_size = ((off_t)UINT64_C(1) << 32) + 123;
    char path[] = "/tmp/musl-bsd-fxstat-XXXXXX";
    struct stat actual;
    struct stat expected;
    int directory_fd;
    int file_fd;
    int pipe_fds[2];
    size_t index;

    for (index = 0; index < sizeof(adapters) / sizeof(adapters[0]); ++index) {
        CHECK(verify_provider(adapters[index].name, (const void*)adapters[index].function,
                              adapters[index].from_compatibility_core) == 0);
    }
    CHECK(verify_provider("__fxstatat", (const void*)__fxstatat, 0) == 0);

    file_fd = mkstemp(path);
    CHECK(file_fd >= 0);
    CHECK(unlink(path) == 0);
    CHECK(fchmod(file_fd, 0640) == 0);
    CHECK(ftruncate(file_fd, large_size) == 0);
    CHECK(fstat(file_fd, &expected) == 0);
    CHECK(expected.st_size == large_size);

    for (index = 0; index < sizeof(adapters) / sizeof(adapters[0]); ++index)
        CHECK(verify_regular_file(&adapters[index], file_fd, &expected) == 0);

    directory_fd = open(".", O_RDONLY | O_DIRECTORY);
    CHECK(directory_fd >= 0);
    for (index = 0; index < sizeof(adapters) / sizeof(adapters[0]); ++index) {
        CHECK(adapters[index].function(1, directory_fd, &actual) == 0);
        CHECK(S_ISDIR(actual.st_mode));
    }
    CHECK(close(directory_fd) == 0);

    CHECK(pipe(pipe_fds) == 0);
    for (index = 0; index < sizeof(adapters) / sizeof(adapters[0]); ++index) {
        CHECK(adapters[index].function(1, pipe_fds[0], &actual) == 0);
        CHECK(S_ISFIFO(actual.st_mode));
    }
    CHECK(close(pipe_fds[0]) == 0);
    CHECK(close(pipe_fds[1]) == 0);

    CHECK(close(file_fd) == 0);
    for (index = 0; index < sizeof(adapters) / sizeof(adapters[0]); ++index) {
        errno = 0;
        CHECK(adapters[index].function(1, file_fd, &actual) == -1);
        CHECK(errno == EBADF);
    }
    CHECK(verify_fxstatat_paths() == 0);
    return 0;
}
