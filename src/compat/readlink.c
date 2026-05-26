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

ssize_t readlink(const char* path, char* buf, size_t len) {
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
        int fd = -1;

        if (linker_path == NULL) {
            linker_path = realpath(MUSL_BSD_MUSL_LINKER_PATH, NULL);
            if (linker_path == NULL)
                return -1;
        }

        if (real_readlink(path, exe_path, sizeof(exe_path)) < 1)
            goto fail;

        if (strcmp(exe_path, linker_path) == 0) {
            char c;
            int arg = 0;
            ssize_t arglen;

            fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
            if (fd < 0)
                goto fail;

            while (arg < 6) {
                if (read(fd, &c, 1) != 1)
                    goto fail;
                if (c == '\0')
                    arg++;
            }

            arglen = read(fd, exe_path, sizeof(exe_path));
            if (arglen < 1)
                goto fail;

            close(fd);
            fd = -1;

            if (exe_path[0] == '\0')
                goto fail;
            if (strnlen(exe_path, (size_t)arglen) == (size_t)arglen)
                goto fail;
        }

        return stpncpy(buf, exe_path, len) - buf;

    fail:
        if (fd >= 0)
            close(fd);
        exe_path[0] = '\0';
        errno = EIO;
        return -1;
    }

    return stpncpy(buf, exe_path, len) - buf;
}
