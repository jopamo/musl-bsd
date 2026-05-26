#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>

int __register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void), void* dso_handle) {
    (void)dso_handle;
    return pthread_atfork(prepare, parent, child);
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
    if (geteuid() != getuid() || getegid() != getgid())
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
