#ifndef MUSL_BSD_OVERLAY_DLFCN_H
#include_next <dlfcn.h>

#define MUSL_BSD_OVERLAY_DLFCN_H

#ifndef MUSL_BSD_LMID_T_DEFINED
#define MUSL_BSD_LMID_T_DEFINED
typedef long int Lmid_t;
#endif

#ifndef LM_ID_BASE
#define LM_ID_BASE ((Lmid_t)0)
#endif

#ifndef LM_ID_NEWLM
#define LM_ID_NEWLM ((Lmid_t) - 1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

void* dlmopen(Lmid_t lmid, const char* filename, int flags);
void* dlvsym(void* handle, const char* symbol, const char* version);

#ifdef __cplusplus
}
#endif

#endif
