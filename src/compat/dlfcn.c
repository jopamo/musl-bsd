#include <dlfcn.h>

void* dlmopen(Lmid_t lmid, const char* filename, int flags) {
    (void)lmid;
    return dlopen(filename, flags);
}

void* dlvsym(void* handle, const char* symbol, const char* version) {
    (void)version;
    return dlsym(handle, symbol);
}
