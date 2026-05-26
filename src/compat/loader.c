#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 16384
#endif

#ifndef MUSL_BSD_GLIBC_LOADER_NAME
#error MUSL_BSD_GLIBC_LOADER_NAME must be defined
#endif

#ifndef MUSL_BSD_MUSL_LINKER_PATH
#error MUSL_BSD_MUSL_LINKER_PATH must be defined
#endif

#ifndef MUSL_BSD_PRELOAD_PATH
#error MUSL_BSD_PRELOAD_PATH must be defined
#endif

static size_t loader_strlcpy(char* dst, const char* src, size_t size) {
    size_t src_len = strlen(src);

    if (size != 0) {
        size_t copy_len = src_len;

        if (copy_len >= size)
            copy_len = size - 1;

        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }

    return src_len;
}

static size_t loader_strlcat(char* dst, const char* src, size_t size) {
    size_t dst_len = strnlen(dst, size);
    size_t src_len = strlen(src);

    if (dst_len == size)
        return size + src_len;

    return dst_len + loader_strlcpy(dst + dst_len, src, size - dst_len);
}

static const char* musl_bsd_preload_path(void) {
    const char* path = getenv("MUSL_BSD_PRELOAD_PATH");

    if (path != NULL && path[0] != '\0')
        return path;

    return MUSL_BSD_PRELOAD_PATH;
}

static void usage(void) {
    printf("This is the musl-bsd glibc ELF interpreter stub.\n");
    printf("You are not meant to run this directly.\n");
}

int main(int argc, char* argv[], char* envp[]) {
    char** new_argv;
    char preload[PATH_MAX] = "";
    char target[PATH_MAX] = "";
    const char* extra_preload = getenv("LD_PRELOAD");
    const char* compat_preload = musl_bsd_preload_path();
    ssize_t len;

    new_argv = calloc((size_t)argc + 7U, sizeof(char*));
    if (new_argv == NULL) {
        perror("calloc");
        return EXIT_FAILURE;
    }

    loader_strlcpy(preload, compat_preload, sizeof(preload));
    if (extra_preload != NULL && extra_preload[0] != '\0') {
        len = (ssize_t)loader_strlcat(preload, " ", sizeof(preload));
        if ((size_t)len >= sizeof(preload)) {
            fputs("too many preloaded libraries\n", stderr);
            return EXIT_FAILURE;
        }
        len = (ssize_t)loader_strlcat(preload, extra_preload, sizeof(preload));
        if ((size_t)len >= sizeof(preload)) {
            fputs("too many preloaded libraries\n", stderr);
            return EXIT_FAILURE;
        }
    }

    len = readlink("/proc/self/exe", target, sizeof(target) - 1);
    if (len < 0 || len >= (ssize_t)sizeof(target) - 1) {
        perror("readlink");
        return EXIT_FAILURE;
    }
    target[len] = '\0';

    if (strstr(target, MUSL_BSD_GLIBC_LOADER_NAME) != NULL) {
        usage();
        return EXIT_FAILURE;
    }

    new_argv[0] = (char*)MUSL_BSD_GLIBC_LOADER_NAME;
    new_argv[1] = (char*)"--argv0";
    new_argv[2] = argv[0];
    new_argv[3] = (char*)"--preload";
    new_argv[4] = preload;
    new_argv[5] = (char*)"--";
    new_argv[6] = target;
    for (int i = 1; i < argc; ++i)
        new_argv[i + 6] = argv[i];

    execve(MUSL_BSD_MUSL_LINKER_PATH, new_argv, envp);
    perror("execve");
    return EXIT_FAILURE;
}
