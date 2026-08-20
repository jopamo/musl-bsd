#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

extern int backtrace(void** buffer, int size);

#if defined(__x86_64__) && __SIZEOF_POINTER__ == 8
typedef char glibc_backtrace_int_abi[(sizeof(int) == 4) ? 1 : -1];
typedef char glibc_backtrace_pointer_abi[(sizeof(void*) == 8) ? 1 : -1];
#endif

#define CHECK(condition)  \
    do {                  \
        if (!(condition)) \
            return 1;     \
    } while (0)

#define NOINLINE __attribute__((noinline))

static volatile int trace_barrier;

NOINLINE int capture_trace(void** frames, int capacity) {
    int count = backtrace(frames, capacity);

    trace_barrier = count;
    return count;
}

NOINLINE int trace_layer(void** frames, int capacity) {
    int count = capture_trace(frames, capacity);

    trace_barrier = count;
    return count;
}

static int frame_has_symbol(void* frame, const char* expected) {
    Dl_info info;

    memset(&info, 0, sizeof(info));
    return dladdr(frame, &info) != 0 && info.dli_sname != NULL && strcmp(info.dli_sname, expected) == 0;
}

int main(void) {
    void* const sentinel = (void*)(uintptr_t)1;
    void* frames[16];
    int count;

    for (size_t index = 0; index < sizeof(frames) / sizeof(frames[0]); ++index)
        frames[index] = sentinel;
    errno = EDOM;
    count = trace_layer(frames, 16);
    CHECK(count >= 2 && count <= 16);
    CHECK(errno == EDOM);
    CHECK(frame_has_symbol(frames[0], "capture_trace"));
    CHECK(frame_has_symbol(frames[1], "trace_layer"));

    frames[0] = sentinel;
    frames[1] = sentinel;
    errno = ERANGE;
    CHECK(capture_trace(frames, 1) == 1);
    CHECK(frames[0] != sentinel);
    CHECK(frames[1] == sentinel);
    CHECK(errno == ERANGE);

    frames[0] = sentinel;
    errno = EINVAL;
    CHECK(backtrace(frames, 0) == 0);
    CHECK(frames[0] == sentinel);
    CHECK(errno == EINVAL);
    CHECK(backtrace(frames, -1) == 0);
    CHECK(frames[0] == sentinel);
    CHECK(errno == EINVAL);
    return 0;
}
