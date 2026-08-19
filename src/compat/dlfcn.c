#include <dlfcn.h>
#include <link.h>
#include <stddef.h>

#ifndef RTLD_DL_LINKMAP
#define RTLD_DL_LINKMAP 2
#endif

void* dlmopen(Lmid_t lmid, const char* filename, int flags) {
    (void)lmid;
    return dlopen(filename, flags);
}

void* dlvsym(void* handle, const char* symbol, const char* version) {
    (void)version;
    return dlsym(handle, symbol);
}

/*
 * glibc exposes dladdr1() as an extension to dladdr().  NVIDIA's GLX
 * runtime uses the RTLD_DL_LINKMAP form to find the link map for a symbol.
 * musl has the two primitives needed to implement that form, but does not
 * expose dladdr1 itself.
 *
 * Keep RTLD_DL_SYMENT deliberately unsupported.  Returning a fabricated
 * ElfW(Sym) would be worse than reporting failure because callers may use
 * the result for symbol-size arithmetic.
 */
int dladdr1(const void* address, Dl_info* info, void** extra_info, int flags) {
    if (extra_info == NULL)
        return 0;

    if (dladdr(address, info) == 0)
        return 0;

    if (flags != RTLD_DL_LINKMAP)
        return 0;

    void* handle = dlopen(info->dli_fname, RTLD_LAZY | RTLD_NOLOAD);
    if (handle != NULL) {
        int result = dlinfo(handle, RTLD_DI_LINKMAP, extra_info) == 0;
        dlclose(handle);
        return result;
    }

    /*
     * musl does not accept the main executable's pathname with RTLD_NOLOAD;
     * dlopen(NULL) is its handle for that object.  Only use that fallback
     * when the returned link map is the object dladdr() identified.
     */
    handle = dlopen(NULL, RTLD_LAZY);
    if (handle == NULL)
        return 0;

    struct link_map* map = NULL;
    int result = dlinfo(handle, RTLD_DI_LINKMAP, &map) == 0 &&
                 map != NULL &&
                 map->l_addr == (ElfW(Addr))info->dli_fbase;
    if (result)
        *extra_info = map;
    dlclose(handle);
    return result;
}
