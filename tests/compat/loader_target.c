#include <dlfcn.h>
#include <link.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int append_trace(const char* event) {
    const char* path = getenv("MUSL_BSD_TEST_TRACE");
    FILE* stream;

    if (path == NULL || path[0] == '\0')
        return 0;
    stream = fopen(path, "a");
    if (stream == NULL)
        return -1;
    fprintf(stream, "%s\n", event);
    return fclose(stream);
}

__attribute__((constructor)) static void target_constructor(void) {
    append_trace("target-constructor");
}

__attribute__((destructor)) static void target_destructor(void) {
    append_trace("target-destructor");
}

struct preload_order {
    const char* core;
    const char* user;
    int index;
    int core_index;
    int user_index;
};

static int find_preloads(struct dl_phdr_info* info, size_t size, void* data) {
    struct preload_order* order = data;

    (void)size;
    if (strstr(info->dlpi_name, order->core) != NULL)
        order->core_index = order->index;
    if (strstr(info->dlpi_name, order->user) != NULL)
        order->user_index = order->index;
    order->index++;
    return 0;
}

static int check_startup(int argc, char** argv) {
    const char* expected_argv0 = getenv("MUSL_BSD_EXPECT_ARGV0");

    if (expected_argv0 == NULL || strcmp(argv[0], expected_argv0) != 0)
        return 10;
    if (argc != 4 || strcmp(argv[2], "alpha") != 0 || strcmp(argv[3], "two words") != 0)
        return 11;
    return 0;
}

static int check_facade_owner(const char* path) {
    Dl_info info;
    void* handle;
    unsigned long (*probe)(void);
    int result = 0;

    handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL)
        return 20;
    probe = (unsigned long (*)(void))dlsym(handle, "musl_bsd_glibc_probe");
    memset(&info, 0, sizeof(info));
    if (probe == NULL || probe() != 0x4d42534400020000UL || dladdr((const void*)probe, &info) == 0 ||
        info.dli_fname == NULL || strstr(info.dli_fname, "libutil.so.1") == NULL)
        result = 21;
    if (dlclose(handle) != 0)
        result = 22;
    return result;
}

static int check_lifecycle(const char* plugin_path) {
    void* handle;
    int (*entry)(void);

    if (append_trace("main") != 0)
        return 30;
    handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 31;
    }
    entry = (int (*)(void))dlsym(handle, "loader_plugin_entry");
    if (entry == NULL || entry() != 73)
        return 32;
    if (dlclose(handle) != 0)
        return 33;
    if (append_trace("after-dlclose") != 0)
        return 34;
    return 0;
}

static int check_load(const char* dso_path) {
    void* handle = dlopen(dso_path, RTLD_NOW | RTLD_LOCAL);

    if (handle == NULL) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 70;
    }
    if (dlclose(handle) != 0) {
        fprintf(stderr, "dlclose: %s\n", dlerror());
        return 71;
    }
    return 0;
}

static int default_symbol_is_absent(const char* symbol) {
    const char* error;
    void* address;

    dlerror();
    address = dlsym(RTLD_DEFAULT, symbol);
    error = dlerror();
    return address == NULL && error != NULL;
}

static int lookup_symbol(void* handle, const char* symbol, void** address) {
    const char* error;

    dlerror();
    *address = dlsym(handle, symbol);
    error = dlerror();
    if (*address != NULL && error == NULL)
        return 0;
    fprintf(stderr, "dlsym(%s): %s\n", symbol, error == NULL ? "null address" : error);
    return -1;
}

static int check_global_load(const char* dso_path, int symbol_count, char** symbols) {
    void** addresses;
    void* global_handle;
    void* local_handle;
    int result = 0;

    addresses = calloc((size_t)symbol_count, sizeof(*addresses));
    if (addresses == NULL)
        return 80;
    for (int i = 0; i < symbol_count; ++i) {
        if (!default_symbol_is_absent(symbols[i])) {
            fprintf(stderr, "%s was globally visible before dlopen\n", symbols[i]);
            result = 81;
            goto free_addresses;
        }
    }
    local_handle = dlopen(dso_path, RTLD_NOW | RTLD_LOCAL);
    if (local_handle == NULL) {
        fprintf(stderr, "local dlopen: %s\n", dlerror());
        result = 82;
        goto free_addresses;
    }
    for (int i = 0; i < symbol_count; ++i) {
        if (lookup_symbol(local_handle, symbols[i], &addresses[i]) != 0) {
            result = 83;
            goto close_local;
        }
        if (!default_symbol_is_absent(symbols[i])) {
            fprintf(stderr, "%s escaped RTLD_LOCAL scope\n", symbols[i]);
            result = 84;
            goto close_local;
        }
    }

    global_handle = dlopen(dso_path, RTLD_NOW | RTLD_GLOBAL);
    if (global_handle == NULL) {
        fprintf(stderr, "global dlopen: %s\n", dlerror());
        result = 85;
        goto close_local;
    }
    for (int i = 0; i < symbol_count; ++i) {
        void* published;

        if (lookup_symbol(RTLD_DEFAULT, symbols[i], &published) != 0 || published != addresses[i]) {
            if (published != NULL && published != addresses[i])
                fprintf(stderr, "%s was published from the wrong object\n", symbols[i]);
            result = 86;
            break;
        }
    }
    if (dlclose(global_handle) != 0 && result == 0)
        result = 87;

close_local:
    if (dlclose(local_handle) != 0 && result == 0)
        result = 88;
free_addresses:
    free(addresses);
    return result;
}

static int check_preload_order(void) {
    struct preload_order order = {
        .core = "libmusl-bsd-core.so",
        .user = "libloader-user-preload.so",
        .core_index = -1,
        .user_index = -1,
    };

    dl_iterate_phdr(find_preloads, &order);
    return order.core_index >= 0 && order.user_index > order.core_index ? 0 : 40;
}

static int check_recurse(char** argv, const char* dso_path) {
    const char* stage = getenv("MUSL_BSD_LOADER_STAGE");

    if (stage != NULL) {
        if (strcmp(stage, "reexec") != 0)
            return 60;
        return dso_path == NULL ? 0 : check_load(dso_path);
    }
    if (setenv("MUSL_BSD_LOADER_STAGE", "reexec", 1) != 0)
        return 61;
    execv("/proc/self/exe", argv);
    return 62;
}

int main(int argc, char** argv) {
    if (argc < 2)
        return 2;
    if (strcmp(argv[1], "startup") == 0)
        return check_startup(argc, argv);
    if (strcmp(argv[1], "facade-owner") == 0 && argc == 3)
        return check_facade_owner(argv[2]);
    if (strcmp(argv[1], "lifecycle") == 0 && argc == 3)
        return check_lifecycle(argv[2]);
    if (strcmp(argv[1], "load") == 0 && argc == 3)
        return check_load(argv[2]);
    if (strcmp(argv[1], "global-load") == 0 && argc >= 4)
        return check_global_load(argv[2], argc - 3, argv + 3);
    if (strcmp(argv[1], "exit") == 0 && argc == 3)
        return atoi(argv[2]);
    if (strcmp(argv[1], "signal") == 0) {
        raise(SIGTERM);
        return 50;
    }
    if (strcmp(argv[1], "preload-order") == 0)
        return check_preload_order();
    if (strcmp(argv[1], "recurse") == 0)
        return check_recurse(argv, NULL);
    if (strcmp(argv[1], "recurse-load") == 0 && argc == 3)
        return check_recurse(argv, argv[2]);
    return 3;
}
