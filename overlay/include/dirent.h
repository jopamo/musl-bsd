#ifndef MUSL_BSD_OVERLAY_DIRENT_H
#include_next <dirent.h>

#define MUSL_BSD_OVERLAY_DIRENT_H

#ifndef dirent64
#define dirent64 dirent
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef alphasort64
int alphasort64(const struct dirent64 **left,
		const struct dirent64 **right);
#define alphasort64 alphasort
#endif

#ifndef scandir64
int scandir64(const char *path, struct dirent64 ***namelist,
	      int (*filter)(const struct dirent64 *entry),
	      int (*compare)(const struct dirent64 **left,
			     const struct dirent64 **right));
#define scandir64 scandir
#endif

#ifdef __cplusplus
}
#endif

#endif
