#include "preload_policy.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char* musl_bsd_compatibility_path(const char* variable, const char* configured) {
    const char* value = getenv(variable);

    return value != NULL && value[0] != '\0' ? value : configured;
}

char* musl_bsd_preload_list(const char* core, const char* nvidia_tls, const char* user) {
    const char* entries[] = {core, nvidia_tls, user};
    size_t lengths[3] = {0};
    size_t count = 0;
    size_t total = 1;
    char* list;
    char* output;

    if (core == NULL || core[0] == '\0') {
        errno = EINVAL;
        return NULL;
    }
    /*
     * NVIDIA TLS is one policy-controlled preload, not another user list.
     * Requiring an absolute, delimiter-free path keeps its identity and
     * position unambiguous.
     */
    if (nvidia_tls != NULL && nvidia_tls[0] != '\0' && (nvidia_tls[0] != '/' || strchr(nvidia_tls, ':') != NULL)) {
        errno = EINVAL;
        return NULL;
    }

    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
        if (entries[i] == NULL || entries[i][0] == '\0')
            continue;
        lengths[i] = strlen(entries[i]);
        size_t separator = count != 0;
        if (total > SIZE_MAX - separator || lengths[i] > SIZE_MAX - total - separator) {
            errno = EOVERFLOW;
            return NULL;
        }
        total += lengths[i] + (count != 0);
        count++;
    }

    list = malloc(total);
    if (list == NULL)
        return NULL;
    output = list;
    count = 0;
    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
        if (lengths[i] == 0)
            continue;
        if (count++ != 0)
            *output++ = ':';
        memcpy(output, entries[i], lengths[i]);
        output += lengths[i];
    }
    *output = '\0';
    return list;
}
