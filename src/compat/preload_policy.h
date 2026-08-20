#ifndef MUSL_BSD_PRELOAD_POLICY_H
#define MUSL_BSD_PRELOAD_POLICY_H

const char* musl_bsd_compatibility_path(const char* variable, const char* configured);
char* musl_bsd_preload_list(const char* core, const char* nvidia_tls, const char* user);

#endif
