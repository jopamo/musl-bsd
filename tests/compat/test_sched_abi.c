#include <dlfcn.h>
#include <errno.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern cpu_set_t* __sched_cpualloc(size_t count);

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
    Dl_info info;
    size_t index;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)allocate_cpu_set, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") != NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__sched_cpualloc") == (void*)allocate_cpu_set);

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
        CPU_FREE(set);
    }
    return 0;
}
