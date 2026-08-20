#include "preload_policy.h"

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef MUSL_BSD_GLIBC_LOADER_NAME
#error MUSL_BSD_GLIBC_LOADER_NAME must be defined
#endif

#ifndef MUSL_BSD_MUSL_LINKER_PATH
#error MUSL_BSD_MUSL_LINKER_PATH must be defined
#endif

#ifndef MUSL_BSD_PRELOAD_PATH
#error MUSL_BSD_PRELOAD_PATH must be defined
#endif

#ifndef MUSL_BSD_LIBRARY_PATH
#error MUSL_BSD_LIBRARY_PATH must be defined
#endif

static char* preload_list(void) {
    const char* core = musl_bsd_compatibility_path("MUSL_BSD_PRELOAD_PATH", MUSL_BSD_PRELOAD_PATH);
    const char* nvidia_tls = getenv("MUSL_BSD_NVIDIA_TLS_PATH");
    const char* user = getenv("LD_PRELOAD");

    return musl_bsd_preload_list(core, nvidia_tls, user);
}

static int (*real_execve)(const char* pathname, char* const argv[], char* const envp[]);
static int (*real_execvp)(const char* file, char* const argv[]);

int execve(const char* pathname, char* const argv[], char* const envp[]) {
    if (real_execve == NULL) {
        real_execve = dlsym(RTLD_NEXT, "execve");
        if (real_execve == NULL) {
            errno = ENOSYS;
            return -1;
        }
    }

    if (strcmp(pathname, "/proc/self/exe") == 0) {
        char target[PATH_MAX];
        char** new_argv;
        char* preloads;
        ssize_t len;
        int argc = 0;

        while (argv[argc] != NULL)
            argc++;
        if ((size_t)argc > (SIZE_MAX / sizeof(char*)) - 9) {
            errno = EOVERFLOW;
            return -1;
        }

        len = readlink("/proc/self/exe", target, sizeof(target) - 1);
        if (len < 0)
            return -1;
        target[len] = '\0';

        preloads = preload_list();
        if (preloads == NULL)
            return -1;

        new_argv = calloc((size_t)argc + 9, sizeof(char*));
        if (new_argv == NULL) {
            free(preloads);
            return -1;
        }

        new_argv[0] = (char*)MUSL_BSD_MUSL_LINKER_PATH;
        new_argv[1] = (char*)"--preload";
        new_argv[2] = preloads;
        new_argv[3] = (char*)"--library-path";
        new_argv[4] = (char*)musl_bsd_compatibility_path("MUSL_BSD_LIBRARY_PATH", MUSL_BSD_LIBRARY_PATH);
        new_argv[5] = (char*)"--argv0";
        new_argv[6] = argv[0];
        new_argv[7] = (char*)"--";
        new_argv[8] = target;
        for (int i = 1; i < argc; ++i)
            new_argv[i + 8] = argv[i];

        int result = real_execve(MUSL_BSD_MUSL_LINKER_PATH, new_argv, envp);
        int saved_errno = errno;
        free(new_argv);
        free(preloads);
        errno = saved_errno;
        return result;
    }

    return real_execve(pathname, argv, envp);
}

extern char** environ;

int execv(const char* pathname, char* const argv[]) {
    return execve(pathname, argv, environ);
}

int execvp(const char* file, char* const argv[]) {
    if (strcmp(file, "/proc/self/exe") == 0)
        return execv(file, argv);

    if (real_execvp == NULL) {
        real_execvp = dlsym(RTLD_NEXT, "execvp");
        if (real_execvp == NULL) {
            errno = ENOSYS;
            return -1;
        }
    }

    return real_execvp(file, argv);
}
