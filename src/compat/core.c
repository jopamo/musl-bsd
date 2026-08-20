#include <stdint.h>

/*
 * Project-owned ABI anchor used by the private facade DSOs.  Keeping this
 * symbol project-versioned forces a real DT_NEEDED edge to the common core.
 */
unsigned long musl_bsd_core_abi(void) {
    return (unsigned long)UINT64_C(0x4d42534400020000);
}
