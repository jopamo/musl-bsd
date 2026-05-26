#include <gnu/libc-version.h>

#include <stdlib.h>

const char* gnu_get_libc_release(void) {
    return "stable";
}

const char* gnu_get_libc_version(void) {
    const char* version = getenv("GLIBC_FAKE_VERSION");

    if (version == NULL || version[0] == '\0')
        return "2.17";

    return version;
}
