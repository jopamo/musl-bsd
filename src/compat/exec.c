#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
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

static const char* musl_bsd_preload_path(void) {
    const char* path = getenv("MUSL_BSD_PRELOAD_PATH");

    if (path != NULL && path[0] != '\0')
        return path;

    return MUSL_BSD_PRELOAD_PATH;
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
        char target[PATH_MAX] = "";
        char** new_argv;
        ssize_t len;
        int argc = 0;

        while (argv[argc] != NULL)
            argc++;

        len = readlink("/proc/self/exe", target, sizeof(target));
        if (len < 0 || len == (ssize_t)sizeof(target)) {
            errno = ENOMEM;
            return -1;
        }

        new_argv = calloc((size_t)argc + 7U, sizeof(char*));
        if (new_argv == NULL)
            return -1;

        new_argv[0] = (char*)MUSL_BSD_GLIBC_LOADER_NAME;
        new_argv[1] = (char*)"--argv0";
        new_argv[2] = argv[0];
        new_argv[3] = (char*)"--preload";
        new_argv[4] = (char*)musl_bsd_preload_path();
        new_argv[5] = (char*)"--";
        new_argv[6] = target;
        for (int i = 1; i < argc; ++i)
            new_argv[i + 6] = argv[i];

        return real_execve(MUSL_BSD_MUSL_LINKER_PATH, new_argv, envp);
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
