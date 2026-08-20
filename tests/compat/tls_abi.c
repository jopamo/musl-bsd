#include <stdint.h>

__thread uintptr_t tls_value;

uintptr_t* tls_value_address(void) {
    return &tls_value;
}
