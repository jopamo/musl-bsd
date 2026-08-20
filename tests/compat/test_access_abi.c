#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int (*access_function)(const char* path, int mode);

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "access ABI test failed at line %d\n", __LINE__); \
            return 1;                                                         \
        }                                                                     \
    } while (0)

static int verify_access(access_function function) {
#define CHECK_ACCESS(condition)                                               \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "access ABI test failed at line %d\n", __LINE__); \
            goto cleanup;                                                     \
        }                                                                     \
    } while (0)
    char directory_path[] = "/tmp/musl-bsd-access-XXXXXX";
    char file_path[PATH_MAX];
    char link_path[PATH_MAX];
    char missing_path[PATH_MAX];
    int directory_created = 0;
    int file_fd = -1;
    int length;
    int result = 1;

    CHECK_ACCESS(mkdtemp(directory_path) != NULL);
    directory_created = 1;
    length = snprintf(file_path, sizeof(file_path), "%s/file", directory_path);
    CHECK_ACCESS(length > 0);
    CHECK_ACCESS((size_t)length < sizeof(file_path));
    length = snprintf(link_path, sizeof(link_path), "%s/link", directory_path);
    CHECK_ACCESS(length > 0);
    CHECK_ACCESS((size_t)length < sizeof(link_path));
    length = snprintf(missing_path, sizeof(missing_path), "%s/missing", directory_path);
    CHECK_ACCESS(length > 0);
    CHECK_ACCESS((size_t)length < sizeof(missing_path));

    file_fd = open(file_path, O_CREAT | O_EXCL | O_RDWR, 0600);
    CHECK_ACCESS(file_fd >= 0);
    CHECK_ACCESS(close(file_fd) == 0);
    file_fd = -1;
    CHECK_ACCESS(symlink("file", link_path) == 0);

    errno = EDOM;
    CHECK_ACCESS(function(file_path, F_OK) == 0);
    CHECK_ACCESS(errno == EDOM);
    errno = ERANGE;
    CHECK_ACCESS(function(file_path, R_OK | W_OK) == 0);
    CHECK_ACCESS(errno == ERANGE);
    errno = E2BIG;
    CHECK_ACCESS(function(directory_path, R_OK | W_OK | X_OK) == 0);
    CHECK_ACCESS(errno == E2BIG);
    errno = ENOTTY;
    CHECK_ACCESS(function(link_path, R_OK) == 0);
    CHECK_ACCESS(errno == ENOTTY);
    errno = EAGAIN;
    CHECK_ACCESS(function(file_path, 0) == 0);
    CHECK_ACCESS(errno == EAGAIN);

    errno = 0;
    CHECK_ACCESS(function(file_path, X_OK) == -1);
    CHECK_ACCESS(errno == EACCES);
    errno = 0;
    CHECK_ACCESS(function(missing_path, F_OK) == -1);
    CHECK_ACCESS(errno == ENOENT);
    errno = 0;
    CHECK_ACCESS(function(file_path, R_OK | 8) == -1);
    CHECK_ACCESS(errno == EINVAL);
    result = 0;

cleanup:
    if (file_fd >= 0)
        close(file_fd);
    if (directory_created) {
        unlink(link_path);
        unlink(file_path);
        rmdir(directory_path);
    }
#undef CHECK_ACCESS
    return result;
}

int main(void) {
    access_function function = access;
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "access") == (void*)function);
    CHECK(verify_access(function) == 0);
    return 0;
}
