#include <dlfcn.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define THREAD_COUNT 8

struct worker {
    thrd_t thread;
    uintptr_t marker;
    uintptr_t* address;
    atomic_int ready;
};

static void* tls_handle;
static const char* tls_symbol;
static atomic_int release_workers;

static int worker_main(void* argument) {
    struct worker* worker = argument;
    uintptr_t* slot = dlsym(tls_handle, tls_symbol);
    int result = 0;

    if (slot == NULL) {
        result = 1;
    }
    else {
        worker->address = slot;
        *slot = worker->marker;
    }
    atomic_store_explicit(&worker->ready, 1, memory_order_release);
    while (!atomic_load_explicit(&release_workers, memory_order_acquire))
        thrd_yield();
    if (result != 0)
        return result;
    if (dlsym(tls_handle, tls_symbol) != slot)
        return 3;
    return *slot == worker->marker ? 0 : 2;
}

int main(int argc, char** argv) {
    struct worker workers[THREAD_COUNT];
    uintptr_t main_marker = UINT64_C(0x4d4253444e56544c);
    uintptr_t* main_slot;
    uintptr_t saved_main;
    size_t created = 0;
    int result = 1;

    if (argc != 3)
        return 2;
    tls_handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (tls_handle == NULL) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 3;
    }
    tls_symbol = argv[2];
    main_slot = dlsym(tls_handle, tls_symbol);
    if (main_slot == NULL) {
        fprintf(stderr, "dlsym: %s\n", dlerror());
        dlclose(tls_handle);
        return 4;
    }
    saved_main = *main_slot;
    *main_slot = main_marker;

    for (uintptr_t i = 0; i < THREAD_COUNT; ++i) {
        workers[i].marker = UINT64_C(0x100000000) + i;
        workers[i].address = NULL;
        atomic_init(&workers[i].ready, 0);
        if (thrd_create(&workers[i].thread, worker_main, &workers[i]) != thrd_success)
            goto release;
        created++;
    }
    for (size_t i = 0; i < THREAD_COUNT; ++i)
        while (!atomic_load_explicit(&workers[i].ready, memory_order_acquire))
            thrd_yield();
    if (*main_slot != main_marker)
        goto release;
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        if (workers[i].address == NULL || workers[i].address == main_slot)
            goto release;
        for (size_t j = 0; j < i; ++j)
            if (workers[i].address == workers[j].address)
                goto release;
    }
    result = 0;

release:
    atomic_store_explicit(&release_workers, 1, memory_order_release);
    for (size_t i = 0; i < created; ++i) {
        int thread_result;
        if (thrd_join(workers[i].thread, &thread_result) != thrd_success || thread_result != 0)
            result = 1;
    }
    *main_slot = saved_main;
    if (dlclose(tls_handle) != 0)
        result = 1;
    return result;
}
