#include "loader_policy.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/auxv.h>
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

#define LOADER_FAILURE 126
#define EXEC_FAILURE 127

static const char* basename_of(const char* path) {
    const char* slash = strrchr(path, '/');

    return slash == NULL ? path : slash + 1;
}

static const char* compatibility_path(const char* variable, const char* configured) {
    const char* value = getenv(variable);

    return value != NULL && value[0] != '\0' ? value : configured;
}

static char* preload_list(const char* core, const char* user) {
    size_t core_len = strlen(core);
    size_t user_len = user == NULL ? 0 : strlen(user);
    char* list;

    if (user_len == 0)
        return strdup(core);
    if (core_len > SIZE_MAX - user_len - 2) {
        errno = EOVERFLOW;
        return NULL;
    }

    list = malloc(core_len + user_len + 2);
    if (list == NULL)
        return NULL;

    memcpy(list, core, core_len);
    list[core_len] = ':';
    memcpy(list + core_len + 1, user, user_len + 1);
    return list;
}

int main(int argc, char* argv[], char* envp[]) {
    unsigned long at_secure;
    const char* target;
    const char* core_preload;
    const char* library_path;
    const char* user_preload;
    char* preloads;
    char** new_argv;

    errno = 0;
    at_secure = getauxval(AT_SECURE);
    if (at_secure == 0 && errno == ENOENT) {
        fputs(
            "musl-bsd loader: kernel did not provide AT_SECURE; "
            "refusing to continue\n",
            stderr);
        return LOADER_FAILURE;
    }

    /*
     * This is the policy boundary.  No environment-controlled compatibility
     * state is read before the kernel's secure-execution decision.
     */
    if (musl_bsd_loader_is_secure(at_secure, getuid(), geteuid(), getgid(), getegid())) {
        fputs(
            "musl-bsd loader: secure execution is unsupported "
            "(AT_SECURE); refusing to continue\n",
            stderr);
        return LOADER_FAILURE;
    }

    target = (const char*)getauxval(AT_EXECFN);
    if (target == NULL || target[0] == '\0') {
        fputs("musl-bsd loader: kernel did not provide AT_EXECFN\n", stderr);
        return LOADER_FAILURE;
    }

    if (strcmp(basename_of(target), MUSL_BSD_GLIBC_LOADER_NAME) == 0) {
        fprintf(stderr,
                "musl-bsd loader: refusing direct or recursive execution of "
                "%s\n",
                MUSL_BSD_GLIBC_LOADER_NAME);
        return LOADER_FAILURE;
    }

    core_preload = compatibility_path("MUSL_BSD_PRELOAD_PATH", MUSL_BSD_PRELOAD_PATH);
    library_path = compatibility_path("MUSL_BSD_LIBRARY_PATH", MUSL_BSD_LIBRARY_PATH);
    user_preload = getenv("LD_PRELOAD");

    /*
     * musl's --preload replaces, rather than augments, LD_PRELOAD.  Build the
     * option value without changing the environment: the trusted core is
     * first, followed by user-requested preloads in their original order.
     */
    preloads = preload_list(core_preload, user_preload);
    if (preloads == NULL) {
        fprintf(stderr, "musl-bsd loader: cannot construct preload list: %s\n", strerror(errno));
        return LOADER_FAILURE;
    }

    if ((size_t)argc > (SIZE_MAX / sizeof(char*)) - 9) {
        fputs("musl-bsd loader: argument vector is too large\n", stderr);
        return LOADER_FAILURE;
    }

    new_argv = calloc((size_t)argc + 9, sizeof(char*));
    if (new_argv == NULL) {
        fprintf(stderr,
                "musl-bsd loader: cannot allocate argument vector: "
                "%s\n",
                strerror(errno));
        return LOADER_FAILURE;
    }

    new_argv[0] = (char*)MUSL_BSD_MUSL_LINKER_PATH;
    new_argv[1] = (char*)"--preload";
    new_argv[2] = preloads;
    new_argv[3] = (char*)"--library-path";
    new_argv[4] = (char*)library_path;
    new_argv[5] = (char*)"--argv0";
    new_argv[6] = argv[0];
    new_argv[7] = (char*)"--";
    new_argv[8] = (char*)target;
    for (int i = 1; i < argc; ++i)
        new_argv[i + 8] = argv[i];

    execve(MUSL_BSD_MUSL_LINKER_PATH, new_argv, envp);
    fprintf(stderr, "musl-bsd loader: cannot execute %s: %s\n", MUSL_BSD_MUSL_LINKER_PATH, strerror(errno));
    return EXEC_FAILURE;
}
