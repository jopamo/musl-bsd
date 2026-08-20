#include <dlfcn.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

typedef int (*setjmp_function)(jmp_buf buffer);

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "_setjmp ABI test failed at line %d\n", __LINE__); \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int verify_no_signal_mask_save(void) {
#define CHECK_SETJMP(condition)                                                \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "_setjmp ABI test failed at line %d\n", __LINE__); \
            goto cleanup;                                                      \
        }                                                                      \
    } while (0)
    jmp_buf buffer;
    sigset_t original_mask;
    sigset_t observed_mask;
    sigset_t signal_set;
    volatile int jump_value;
    volatile int mask_changed = 0;
    int result = 1;

    CHECK_SETJMP(sigprocmask(SIG_SETMASK, NULL, &original_mask) == 0);
    CHECK_SETJMP(sigemptyset(&signal_set) == 0);
    CHECK_SETJMP(sigaddset(&signal_set, SIGUSR1) == 0);
    CHECK_SETJMP(sigprocmask(SIG_UNBLOCK, &signal_set, NULL) == 0);
    mask_changed = 1;

    errno = EDOM;
    jump_value = _setjmp(buffer);
    if (jump_value == 0) {
        CHECK_SETJMP(sigprocmask(SIG_BLOCK, &signal_set, NULL) == 0);
        _longjmp(buffer, 9);
    }

    CHECK_SETJMP(jump_value == 9);
    CHECK_SETJMP(errno == EDOM);
    CHECK_SETJMP(sigprocmask(SIG_SETMASK, NULL, &observed_mask) == 0);
    CHECK_SETJMP(sigismember(&observed_mask, SIGUSR1) == 1);
    result = 0;

cleanup:
    if (mask_changed)
        sigprocmask(SIG_SETMASK, &original_mask, NULL);
#undef CHECK_SETJMP
    return result;
}

int main(void) {
    setjmp_function function = _setjmp;
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "_setjmp") == (void*)function);
    CHECK(verify_no_signal_mask_save() == 0);
    return 0;
}
