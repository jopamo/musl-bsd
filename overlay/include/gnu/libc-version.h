#ifndef MUSL_BSD_OVERLAY_GNU_LIBC_VERSION_H
#define MUSL_BSD_OVERLAY_GNU_LIBC_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

const char* gnu_get_libc_release(void);
const char* gnu_get_libc_version(void);

#ifdef __cplusplus
}
#endif

#endif
