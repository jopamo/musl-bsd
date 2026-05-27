#ifndef MUSL_BSD_OVERLAY_MALLOC_H
#include_next <malloc.h>

#define MUSL_BSD_OVERLAY_MALLOC_H

#include <stddef.h>

struct mallinfo {
    int arena;
    int ordblks;
    int smblks;
    int hblks;
    int hblkhd;
    int usmblks;
    int fsmblks;
    int uordblks;
    int fordblks;
    int keepcost;
};

#ifdef __cplusplus
extern "C" {
#endif

struct mallinfo mallinfo(void);
#if __INCLUDE_LEVEL__ == 1
int malloc_trim(size_t pad);
#endif
void mtrace(void);
void muntrace(void);

#ifdef __cplusplus
}
#endif

#endif
