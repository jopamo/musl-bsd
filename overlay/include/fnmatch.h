#ifndef MUSL_BSD_OVERLAY_FNMATCH_H
#include_next <fnmatch.h>

#define MUSL_BSD_OVERLAY_FNMATCH_H

#ifndef FNM_EXTMATCH
#define FNM_EXTMATCH 0
#endif

#endif
