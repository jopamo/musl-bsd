#ifndef MUSL_BSD_PRELOAD_POLICY_H
#define MUSL_BSD_PRELOAD_POLICY_H

/* Returned values borrow envp storage; the first exact name match wins. */
const char* musl_bsd_environment_value(char* const envp[], const char* variable);
const char* musl_bsd_compatibility_path(char* const envp[], const char* variable, const char* configured);
char* musl_bsd_preload_list(const char* core, const char* early, const char* user);

#endif
