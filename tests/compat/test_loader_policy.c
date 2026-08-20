#include "loader_policy.h"

#include <stdio.h>

#define CHECK(condition)                                                          \
    do {                                                                          \
        if (!(condition)) {                                                       \
            fprintf(stderr, "loader policy check failed at line %d\n", __LINE__); \
            return 1;                                                             \
        }                                                                         \
    } while (0)

int main(void) {
    CHECK(!musl_bsd_loader_is_secure(0, 1000, 1000, 1000, 1000));
    CHECK(musl_bsd_loader_is_secure(1, 1000, 1000, 1000, 1000));
    CHECK(musl_bsd_loader_is_secure(0, 1000, 0, 1000, 1000));
    CHECK(musl_bsd_loader_is_secure(0, 1000, 1000, 1000, 0));
    return 0;
}
