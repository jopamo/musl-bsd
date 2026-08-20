#include "loader_policy.h"
#include "preload_policy.h"

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

int main(int argc, char* argv[], char* envp[]) {
    unsigned long at_secure;
    const char* target;
    const char* core_preload;
    const char* nvidia_tls_preload;
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

    core_preload = musl_bsd_compatibility_path("MUSL_BSD_PRELOAD_PATH", MUSL_BSD_PRELOAD_PATH);
    nvidia_tls_preload = getenv("MUSL_BSD_NVIDIA_TLS_PATH");
    library_path = musl_bsd_compatibility_path("MUSL_BSD_LIBRARY_PATH", MUSL_BSD_LIBRARY_PATH);
    user_preload = getenv("LD_PRELOAD");

    /*
     * musl's --preload replaces, rather than augments, LD_PRELOAD.  Build the
     * option value without changing the environment: the trusted core is
     * first, optional NVIDIA initial-exec TLS is second, and user-requested
     * preloads retain their original order after both required entries.
     */
    preloads = musl_bsd_preload_list(core_preload, nvidia_tls_preload, user_preload);
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
