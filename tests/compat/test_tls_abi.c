#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uintptr_t* (*tls_address_function)(void);
typedef void* (*tls_get_addr_function)(size_t* module_offset);

struct worker {
    pthread_t thread;
    uintptr_t marker;
    uintptr_t* address;
    int result;
};

static tls_address_function tls_address;

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            fprintf(stderr, "TLS ABI test failed at line %d\n", __LINE__); \
            return 1;                                                      \
        }                                                                  \
    } while (0)

static void* worker_main(void* argument) {
    struct worker* worker = argument;

    worker->address = tls_address();
    worker->result = 1;
    if (worker->address == NULL)
        return NULL;
    *worker->address = worker->marker;
    worker->result = *worker->address == worker->marker ? 0 : 2;
    return NULL;
}

int main(int argc, char** argv) {
    static const size_t worker_count = 4;
    static const uintptr_t main_marker = UINT64_C(0x4d425344544c5341);
    struct worker workers[4];
    Dl_info info;
    void* handle;
    tls_get_addr_function get_addr;
    uintptr_t* main_address;
    size_t index;
    size_t other;

    CHECK(argc == 2);
    handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    CHECK(handle != NULL);
    tls_address = (tls_address_function)dlsym(handle, "tls_value_address");
    CHECK(tls_address != NULL);
    CHECK(dlsym(handle, "tls_value_address") == (void*)tls_address);

    get_addr = (tls_get_addr_function)dlsym(RTLD_DEFAULT, "__tls_get_addr");
    CHECK(get_addr != NULL);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)get_addr, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__tls_get_addr") == (void*)get_addr);
    errno = EDOM;
    main_address = tls_address();
    CHECK(main_address != NULL);
    CHECK(errno == EDOM);
    *main_address = main_marker;

    for (index = 0; index < worker_count; ++index) {
        workers[index].marker = (uintptr_t)UINT64_C(0x100000000) + index;
        workers[index].address = NULL;
        workers[index].result = 1;
        CHECK(pthread_create(&workers[index].thread, NULL, worker_main, &workers[index]) == 0);
    }
    for (index = 0; index < worker_count; ++index)
        CHECK(pthread_join(workers[index].thread, NULL) == 0);

    CHECK(*main_address == main_marker);
    for (index = 0; index < worker_count; ++index) {
        CHECK(workers[index].result == 0);
        CHECK(workers[index].address != NULL);
        CHECK(workers[index].address != main_address);
        for (other = 0; other < index; ++other)
            CHECK(workers[index].address != workers[other].address);
    }

    CHECK(dlclose(handle) == 0);
    return 0;
}
