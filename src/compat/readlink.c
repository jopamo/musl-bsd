#include "native_lookup.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MUSL_BSD_MUSL_LINKER_PATH
#error MUSL_BSD_MUSL_LINKER_PATH must be defined
#endif

/* The winning path is immutable and retained for the process lifetime. */
static char* cached_exe_path;
static void* readlink_symbol;

static int target_from_cmdline(char* target, size_t size) {
    char cmdline[PATH_MAX * 2];
    char* cursor;
    char* end;
    ssize_t count;
    int fd;
    /* Skip the interpreter's argv[0], then consume options and their values. */
    int skip_value = 1;

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
        if (skip_value) {
            skip_value = 0;
            cursor += item_len + 1;
            continue;
        }
        if (strcmp(cursor, "--") == 0) {
            const char* value = cursor + item_len + 1;
            size_t value_len;

            if (value >= end)
                break;
            value_len = strnlen(value, (size_t)(end - value));
            if (value_len == (size_t)(end - value))
                break;
            if (value_len == 0 || value_len >= size) {
                errno = value_len == 0 ? EIO : ENAMETOOLONG;
                return -1;
            }
            memcpy(target, value, value_len + 1);
            return 0;
        }
        if (strcmp(cursor, "--preload") != 0 && strcmp(cursor, "--library-path") != 0 && strcmp(cursor, "--argv0") != 0)
            break;
        skip_value = 1;
        cursor += item_len + 1;
    }

    errno = EIO;
    return -1;
}

ssize_t readlink(const char* path, char* buf, size_t len) {
    size_t path_len;
    size_t copy_len;

    ssize_t (*real_readlink)(const char*, char*, size_t) = musl_bsd_native_lookup(&readlink_symbol, "readlink");
    if (real_readlink == NULL)
        return -1;

    if (strcmp(path, "/proc/self/exe") != 0)
        return real_readlink(path, buf, len);

    char* exe_path = __atomic_load_n(&cached_exe_path, __ATOMIC_ACQUIRE);
    if (exe_path == NULL) {
        char candidate[PATH_MAX];
        char linker_path[PATH_MAX];
        ssize_t count;

        if (realpath(MUSL_BSD_MUSL_LINKER_PATH, linker_path) == NULL)
            return -1;

        count = real_readlink(path, candidate, sizeof(candidate));
        if (count < 1)
            goto fail;
        if ((size_t)count == sizeof(candidate)) {
            errno = ENAMETOOLONG;
            return -1;
        }
        candidate[count] = '\0';

        if (strcmp(candidate, linker_path) == 0 && target_from_cmdline(candidate, sizeof(candidate)) != 0)
            goto fail;

        exe_path = strdup(candidate);
        if (exe_path == NULL)
            return -1;
        char* expected = NULL;
        if (!__atomic_compare_exchange_n(&cached_exe_path, &expected, exe_path, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
            free(exe_path);
            exe_path = expected;
        }
    }

    path_len = strlen(exe_path);
    copy_len = path_len < len ? path_len : len;
    memcpy(buf, exe_path, copy_len);
    return (ssize_t)copy_len;

fail:
    errno = EIO;
    return -1;
}
