#include <dlfcn.h>
#include <errno.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern cpu_set_t* __sched_cpualloc(size_t count);
extern int __sched_cpucount(size_t size, const cpu_set_t* set);
extern void __sched_cpufree(cpu_set_t* set);

#define CHECK(condition)                                                         \
    do {                                                                         \
        if (!(condition)) {                                                      \
            fprintf(stderr, "scheduler ABI test failed at line %d\n", __LINE__); \
            return 1;                                                            \
        }                                                                        \
    } while (0)

int main(void) {
    static const size_t counts[] = {1, 64, 65, 256, 1024};
    cpu_set_t* (*allocate_cpu_set)(size_t count) = __sched_cpualloc;
    int (*count_cpu_set)(size_t size, const cpu_set_t* set) = __sched_cpucount;
    void (*free_cpu_set)(cpu_set_t* set) = __sched_cpufree;
    static const struct {
        size_t size;
        int expected;
    } count_cases[] = {
        {0, 0}, {1, 1}, {7, 1}, {8, 9}, {64, 10}, {128, 11},
    };
    cpu_set_t storage[128 / sizeof(cpu_set_t)];
    unsigned char* storage_bytes = (unsigned char*)storage;
    Dl_info info;
    size_t index;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)allocate_cpu_set, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") != NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__sched_cpualloc") == (void*)allocate_cpu_set);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)count_cpu_set, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__sched_cpucount") == (void*)count_cpu_set);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)free_cpu_set, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") != NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__sched_cpufree") == (void*)free_cpu_set);

    for (index = 0; index < sizeof(counts) / sizeof(counts[0]); ++index) {
        size_t count = counts[index];
        size_t allocation_size = CPU_ALLOC_SIZE(count);
        cpu_set_t* set;
        unsigned char* bytes;
        size_t byte_index;

        CHECK(allocation_size >= sizeof(unsigned long));
        errno = EDOM;
        set = allocate_cpu_set(count);
        CHECK(set != NULL);
        CHECK(errno == EDOM);
        CHECK(((uintptr_t)set % _Alignof(cpu_set_t)) == 0);

        bytes = (unsigned char*)set;
        for (byte_index = 0; byte_index < allocation_size; ++byte_index)
            CHECK(bytes[byte_index] == 0);

        CPU_SET_S(count - 1, allocation_size, set);
        CHECK(CPU_ISSET_S(count - 1, allocation_size, set));
        CHECK(!CPU_ISSET_S(count, allocation_size, set));
        errno = ENOTTY;
        free_cpu_set(set);
        CHECK(errno == ENOTTY);
    }

    memset(storage, 0, sizeof(storage));
    storage_bytes[0] = 0x01;
    storage_bytes[7] = 0xff;
    storage_bytes[63] = 0x80;
    storage_bytes[127] = 0x01;
    errno = EDOM;
    CHECK(count_cpu_set(0, NULL) == 0);
    CHECK(errno == EDOM);
    for (index = 0; index < sizeof(count_cases) / sizeof(count_cases[0]); ++index) {
        errno = ERANGE;
        CHECK(count_cpu_set(count_cases[index].size, storage) == count_cases[index].expected);
        CHECK(errno == ERANGE);
    }
    return 0;
}
