#ifndef MUSL_BSD_OVERLAY_DIRENT_H
#include_next <dirent.h>

#define MUSL_BSD_OVERLAY_DIRENT_H

#ifndef dirent64
#define dirent64 dirent
#endif

#ifndef alphasort64
#define alphasort64 alphasort
#endif

#ifndef scandir64
#define scandir64 scandir
#endif

#endif
