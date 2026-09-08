#ifndef MUSL_BSD_NATIVE_LOOKUP_H
#define MUSL_BSD_NATIVE_LOOKUP_H

#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>

/* No in-progress state: reentry or fork can make an independent attempt. */
static inline void* musl_bsd_native_lookup(void** cache, const char* name) {
    void* address = __atomic_load_n(cache, __ATOMIC_ACQUIRE);
    if (address == NULL) {
        address = dlsym(RTLD_NEXT, name);
        if (address == NULL) {
            errno = ENOSYS;
            return NULL;
        }
        void* expected = NULL;
        if (!__atomic_compare_exchange_n(cache, &expected, address, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
            address = expected;
    }
    return address;
}

#endif
