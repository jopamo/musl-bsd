#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MUSL_BSD_MUSL_LINKER_PATH
#error MUSL_BSD_MUSL_LINKER_PATH must be defined
#endif

static char exe_path[PATH_MAX];
static char* linker_path;
static ssize_t (*real_readlink)(const char* path, char* buf, size_t len);

static int target_from_cmdline(char* target, size_t size) {
    char cmdline[PATH_MAX * 2];
    char* cursor;
    char* end;
    ssize_t count;
    int fd;

    fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return -1;
    count = read(fd, cmdline, sizeof(cmdline) - 1);
    close(fd);
    if (count < 1) {
        errno = EIO;
        return -1;
    }
    cmdline[count] = '\0';
    end = cmdline + count;

    for (cursor = cmdline; cursor < end;) {
        size_t item_len = strnlen(cursor, (size_t)(end - cursor));

        if (item_len == (size_t)(end - cursor))
            break;
        if (strcmp(cursor, "--") == 0) {
            const char* value = cursor + item_len + 1;
            size_t value_len;

            if (value >= end)
                break;
            value_len = strnlen(value, (size_t)(end - value));
            if (value_len == 0 || value_len >= size) {
                errno = value_len == 0 ? EIO : ENAMETOOLONG;
                return -1;
            }
            memcpy(target, value, value_len + 1);
            return 0;
        }
        cursor += item_len + 1;
    }

    errno = EIO;
    return -1;
}

ssize_t readlink(const char* path, char* buf, size_t len) {
    size_t path_len;
    size_t copy_len;

    if (real_readlink == NULL) {
        real_readlink = dlsym(RTLD_NEXT, "readlink");
        if (real_readlink == NULL) {
            errno = ENOSYS;
            return -1;
        }
    }

    if (strcmp(path, "/proc/self/exe") != 0)
        return real_readlink(path, buf, len);

    if (exe_path[0] == '\0') {
        ssize_t count;

        if (linker_path == NULL) {
            linker_path = realpath(MUSL_BSD_MUSL_LINKER_PATH, NULL);
            if (linker_path == NULL)
                return -1;
        }

        count = real_readlink(path, exe_path, sizeof(exe_path) - 1);
        if (count < 1)
            goto fail;
        exe_path[count] = '\0';

        if (strcmp(exe_path, linker_path) == 0 && target_from_cmdline(exe_path, sizeof(exe_path)) != 0)
            goto fail;
    }

    path_len = strlen(exe_path);
    copy_len = path_len < len ? path_len : len;
    memcpy(buf, exe_path, copy_len);
    return (ssize_t)copy_len;

fail:
    exe_path[0] = '\0';
    errno = EIO;
    return -1;
}
