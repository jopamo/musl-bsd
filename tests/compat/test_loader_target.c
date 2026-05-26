#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <limits.h>

int main(int argc, char** argv) {
    char exe[PATH_MAX];
    const char* stage;
    ssize_t len;

    (void)argc;

    if (strstr(argv[0], "compat_loader_runtime") == NULL)
        return 1;

    len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len < 0 || len >= (ssize_t)sizeof(exe) - 1)
        return 2;
    exe[len] = '\0';

    if (strstr(exe, "compat_loader_runtime") == NULL)
        return 3;
    if (strstr(exe, "ld-linux") != NULL)
        return 4;

    stage = getenv("MUSL_BSD_LOADER_STAGE");
    if (stage == NULL) {
        if (setenv("MUSL_BSD_LOADER_STAGE", "reexec", 1) != 0)
            return 5;
        execv("/proc/self/exe", argv);
        return 6;
    }

    if (strcmp(stage, "reexec") != 0)
        return 7;

    return 0;
}
