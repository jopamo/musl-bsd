#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

#ifdef MUSL_BSD_HAVE_UNWIND
#include <unwind.h>
#endif

int __register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void), void* dso_handle) {
    (void)dso_handle;
    return pthread_atfork(prepare, parent, child);
}

/*
 * NVIDIA's compiler runtime uses glibc's internal key-creation entry point
 * through a weak GLOB_DAT relocation.  musl only exports the public alias,
 * so leaving this unresolved makes the runtime treat TLS initialization as a
 * hard failure and throw std::system_error.
 */
int __pthread_key_create(pthread_key_t* key, void (*destructor)(void*)) {
    return pthread_key_create(key, destructor);
}

int register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void), void* dso_handle) {
    return __register_atfork(prepare, parent, child, dso_handle);
}

cpu_set_t* __sched_cpualloc(size_t count) {
#ifdef CPU_ALLOC
    return CPU_ALLOC(count);
#else
    (void)count;
    errno = ENOSYS;
    return NULL;
#endif
}

void __sched_cpufree(cpu_set_t* set) {
#ifdef CPU_FREE
    CPU_FREE(set);
#else
    free(set);
#endif
}

struct mallinfo mallinfo(void) {
    struct mallinfo info;

    memset(&info, 0, sizeof(info));
    return info;
}

int malloc_trim(size_t pad) {
    (void)pad;
    return 0;
}

void mtrace(void) {}

void muntrace(void) {}

char* __realpath_chk(const char* path, char* resolved_path, size_t resolved_len) {
    assert(path != NULL);
    assert(resolved_path != NULL);
    assert(resolved_len >= PATH_MAX);

    return realpath(path, resolved_path);
}

char* __strdup(const char* string) {
    return strdup(string);
}

char* __strtok_r(char* s, const char* delim, char** save_ptr) {
    return strtok_r(s, delim, save_ptr);
}

#ifdef MUSL_BSD_HAVE_UNWIND
struct backtrace_state {
    void** frames;
    uintptr_t previous_cfa;
    uintptr_t previous_ip;
    int count;
    int capacity;
    int skip_current;
};

static _Unwind_Reason_Code capture_backtrace_frame(struct _Unwind_Context* context, void* argument) {
    struct backtrace_state* state = argument;
    uintptr_t cfa;
    uintptr_t ip;

    if (state->skip_current) {
        state->skip_current = 0;
        return _URC_NO_REASON;
    }

    cfa = _Unwind_GetCFA(context);
    ip = _Unwind_GetIP(context);
    if (state->count > 0 && cfa == state->previous_cfa && ip == state->previous_ip)
        return _URC_END_OF_STACK;

    state->frames[state->count++] = (void*)(uintptr_t)ip;
    state->previous_cfa = cfa;
    state->previous_ip = ip;
    return state->count == state->capacity ? _URC_END_OF_STACK : _URC_NO_REASON;
}
#endif

/*
 * Glcore records up to sixteen return addresses in diagnostic messages.
 * Follow glibc's _Unwind_Backtrace model: omit this function's frame, stop at
 * the caller's capacity, reject a non-progressing unwinder, and remove the
 * null sentinel some unwinders report above _start.
 */
int backtrace(void** buffer, int size) {
#ifdef MUSL_BSD_HAVE_UNWIND
    struct backtrace_state state = {
        .frames = buffer,
        .previous_cfa = 0,
        .previous_ip = 0,
        .count = 0,
        .capacity = size,
        .skip_current = 1,
    };
    int saved_errno = errno;

    if (size <= 0)
        return 0;
    _Unwind_Backtrace(capture_backtrace_frame, &state);
    if (state.count > 0 && state.frames[state.count - 1] == NULL)
        --state.count;
    errno = saved_errno;
    return state.count;
#else
    (void)buffer;
    (void)size;
    return 0;
#endif
}
