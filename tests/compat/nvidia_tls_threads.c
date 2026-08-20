#include <dlfcn.h>
#include <pthread.h>
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
    unsigned int destructor_bit;
    atomic_int ready;
};

typedef int (*key_create_fn)(pthread_key_t*, void (*)(void*));

static void* tls_handle;
static const char* tls_symbol;
static pthread_key_t destructor_key;
static atomic_uint destructor_calls;
static atomic_uint destructor_mask;
static atomic_int release_workers;

static void worker_destructor(void* value) {
    const struct worker* worker = value;

    atomic_fetch_add_explicit(&destructor_calls, 1, memory_order_relaxed);
    atomic_fetch_or_explicit(&destructor_mask, worker->destructor_bit, memory_order_relaxed);
}

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
        if (pthread_setspecific(destructor_key, worker) != 0)
            result = 4;
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
    key_create_fn key_create;
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
    key_create = (key_create_fn)dlsym(RTLD_DEFAULT, "__pthread_key_create");
    if (key_create == NULL) {
        fprintf(stderr, "dlsym(__pthread_key_create): %s\n", dlerror());
        dlclose(tls_handle);
        return 5;
    }
    if (key_create(&destructor_key, worker_destructor) != 0) {
        fputs("__pthread_key_create failed\n", stderr);
        dlclose(tls_handle);
        return 6;
    }
    saved_main = *main_slot;
    *main_slot = main_marker;

    for (uintptr_t i = 0; i < THREAD_COUNT; ++i) {
        workers[i].marker = UINT64_C(0x100000000) + i;
        workers[i].address = NULL;
        workers[i].destructor_bit = 1u << i;
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
    if (atomic_load_explicit(&destructor_calls, memory_order_relaxed) != created ||
        atomic_load_explicit(&destructor_mask, memory_order_relaxed) != (1u << created) - 1u)
        result = 1;
    *main_slot = saved_main;
    if (pthread_key_delete(destructor_key) != 0)
        result = 1;
    if (dlclose(tls_handle) != 0)
        result = 1;
    return result;
}
