#ifndef MUSL_BSD_OVERLAY_SYS_MOUNT_H
#include_next <sys/mount.h>

#define MUSL_BSD_OVERLAY_SYS_MOUNT_H

#ifdef __linux__

#include <fcntl.h>
#include <linux/mount.h>
#include <stddef.h>

/*
 * musl exposes the new mount API kernel constants, but not the glibc-style
 * wrapper functions. Declare libc-like entry points here and provide the
 * implementations from libmusl-bsd-compat so downstream feature probes see
 * real symbols instead of preprocessor-only shims.
 */
#ifdef open_tree
#undef open_tree
#endif

#ifdef move_mount
#undef move_mount
#endif

#ifdef fsopen
#undef fsopen
#endif

#ifdef fsconfig
#undef fsconfig
#endif

#ifdef fsmount
#undef fsmount
#endif

#ifdef fspick
#undef fspick
#endif

#ifdef mount_setattr
#undef mount_setattr
#endif

#ifdef __cplusplus
extern "C" {
#endif

int open_tree(int dfd, const char *path, unsigned int flags);
int move_mount(int from_dfd, const char *from_path, int to_dfd,
	       const char *to_path, unsigned int flags);
int fsopen(const char *fs_name, unsigned int flags);
int fsconfig(int fsfd, unsigned int cmd, const char *key,
	     const void *value, int aux);
int fsmount(int fsfd, unsigned int flags, unsigned int attr_flags);
int fspick(int dfd, const char *path, unsigned int flags);
int mount_setattr(int dfd, const char *path, unsigned int flags,
		  struct mount_attr *attr, size_t size);

#ifdef __cplusplus
}
#endif

#endif

#endif
