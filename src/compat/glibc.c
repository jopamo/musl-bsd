#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <sys/auxv.h>

/*
 * NVIDIA's GLX module probes the old glibc malloc-hook ABI at runtime.  The
 * hooks were removed from modern glibc, and musl has never provided them.
 * Leave them NULL: the module uses their presence as a capability probe and
 * falls back to the normal allocator when they are unset.
 */
void* (*__malloc_hook)(size_t, const void*) = NULL;
void* (*__realloc_hook)(void*, size_t, const void*) = NULL;
void* (*__memalign_hook)(size_t, size_t, const void*) = NULL;
void (*__free_hook)(void*, const void*) = NULL;

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

char* __secure_getenv(const char* name) {
    if (getauxval(AT_SECURE) != 0 || geteuid() != getuid() ||
        getegid() != getgid())
        return NULL;

    return getenv(name);
}

char* secure_getenv(const char* name) {
    return __secure_getenv(name);
}

char* __strdup(const char* string) {
    return strdup(string);
}

char* __strtok_r(char* s, const char* delim, char** save_ptr) {
    return strtok_r(s, delim, save_ptr);
}

/*
 * NVIDIA's error/reporting path references glibc's execinfo entry point.
 * musl does not ship execinfo.h or a public backtrace implementation.  A
 * zero-frame result is the documented failure result for backtrace() and
 * keeps the optional diagnostic path from making an otherwise loadable DSO
 * fail relocation.
 */
int backtrace(void** buffer, int size) {
    (void)buffer;
    (void)size;
    return 0;
}
