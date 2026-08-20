#include <dlfcn.h>
#include <errno.h>
#include <gnu/libc-version.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern unsigned long musl_bsd_core_abi(void);

#define CHECK(condition)  \
    do {                  \
        if (!(condition)) \
            return 1;     \
    } while (0)

int main(void) {
    char* (*public_secure_getenv)(const char*) = secure_getenv;
    const char* value;
    const char* error;
    Dl_info info;

    CHECK(musl_bsd_core_abi() == (unsigned long)UINT64_C(0x4d42534400020000));
    CHECK(setenv("MUSL_BSD_SECURE_ENV", "trusted", 1) == 0);

    errno = EDOM;
    value = public_secure_getenv("MUSL_BSD_SECURE_ENV");
    CHECK(value != NULL && strcmp(value, "trusted") == 0);
    CHECK(value == getenv("MUSL_BSD_SECURE_ENV"));
    CHECK(errno == EDOM);

    errno = ERANGE;
    CHECK(public_secure_getenv("MUSL_BSD_MISSING_ENV") == NULL);
    CHECK(errno == ERANGE);

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)public_secure_getenv, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "secure_getenv") == (void*)public_secure_getenv);

    dlerror();
    errno = EINVAL;
    CHECK(dlsym(RTLD_DEFAULT, "__secure_getenv") == NULL);
    error = dlerror();
    CHECK(error != NULL);
    CHECK(errno == EINVAL);
    CHECK(dlerror() == NULL);

    CHECK(setenv("GLIBC_FAKE_VERSION", "9.9-test", 1) == 0);
    CHECK(strcmp(gnu_get_libc_version(), "9.9-test") == 0);
    CHECK(unsetenv("GLIBC_FAKE_VERSION") == 0);
    CHECK(strcmp(gnu_get_libc_version(), "2.17") == 0);
    return 0;
}
