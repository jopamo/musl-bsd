#include <dlfcn.h>
#include <link.h>
#include <stddef.h>
#include <string.h>

#ifndef RTLD_DL_LINKMAP
#define RTLD_DL_LINKMAP 2
#endif

static void* dynamic_lookup_error(void* handle) {
    /*
     * dlsym of the empty name deterministically returns NULL and publishes a
     * loader error without inventing a second thread-local dlerror channel.
     */
    return dlsym(handle, "");
}

void* dlmopen(Lmid_t lmid, const char* filename, int flags) {
    /*
     * musl has one link-map namespace. Collapsing LM_ID_NEWLM into the base
     * namespace would silently widen symbol visibility and alter ownership.
     */
    if (lmid != LM_ID_BASE)
        return dynamic_lookup_error(RTLD_DEFAULT);
    return dlopen(filename, flags);
}

/*
 * musl resolves symbols by name and cannot select a glibc version definition.
 * Keep that downgrade explicit and bounded to versions observed in the
 * qualified NVIDIA/CUDA graph or in its direct dlvsym probes.
 */
static int dlvsym_version_supported(const char* version) {
    static const char* const versions[] = {
        "GLIBC_2.2.5", "GLIBC_2.3", "GLIBC_2.3.2", "GLIBC_2.3.3", "GLIBC_2.3.4", "GLIBC_2.4",  "GLIBC_2.6",
        "GLIBC_2.7",   "GLIBC_2.9", "GLIBC_2.10",  "GLIBC_2.12",  "GLIBC_2.14",  "GLIBC_2.16", "GLIBC_2.17",
    };

    if (version == NULL)
        return 0;
    for (size_t index = 0; index < sizeof(versions) / sizeof(versions[0]); ++index)
        if (strcmp(version, versions[index]) == 0)
            return 1;
    return 0;
}

void* dlvsym(void* handle, const char* symbol, const char* version) {
    if (!dlvsym_version_supported(version))
        return dynamic_lookup_error(handle);
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
    int result =
        dlinfo(handle, RTLD_DI_LINKMAP, &map) == 0 && map != NULL && map->l_addr == (ElfW(Addr))info->dli_fbase;
    if (result)
        *extra_info = map;
    dlclose(handle);
    return result;
}
