#ifndef MUSL_BSD_OVERLAY_DIRENT_H
#include_next <dirent.h>

#define MUSL_BSD_OVERLAY_DIRENT_H

#ifndef dirent64
#define dirent64 dirent
#endif

#ifdef __cplusplus
extern "C" {
#endif

int alphasort64(const struct dirent** a, const struct dirent** b);
int scandir64(const char* path,
              struct dirent*** namelist,
              int (*filter)(const struct dirent*),
              int (*compar)(const struct dirent**, const struct dirent**));

#ifdef __cplusplus
}
#endif

#endif
