#ifndef MUSL_BSD_OVERLAY_SYS_STAT_H
#include_next <sys/stat.h>

#define MUSL_BSD_OVERLAY_SYS_STAT_H

#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

#ifndef ALLPERMS
#define ALLPERMS (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO)
#endif

#ifndef DEFFILEMODE
#define DEFFILEMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#endif

#endif
