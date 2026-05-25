#ifndef MUSL_BSD_OVERLAY_SYS_MOUNT_H
#define MUSL_BSD_OVERLAY_SYS_MOUNT_H

#include_next <sys/mount.h>

#ifdef __linux__

#include <fcntl.h>
#include <linux/mount.h>
#include <sys/syscall.h>
#include <unistd.h>

/*
 * musl exposes the new mount API syscall numbers and kernel constants, but
 * not the glibc-style wrapper functions. Provide macro shims so software that
 * includes <sys/mount.h> keeps building on musl without package-local hacks.
 */
#ifndef open_tree
# ifdef SYS_open_tree
#  define open_tree(dfd, path, flags) \
	((int)syscall(SYS_open_tree, (dfd), (path), (flags)))
# endif
#endif

#ifndef move_mount
# ifdef SYS_move_mount
#  define move_mount(from_dfd, from_path, to_dfd, to_path, flags) \
	((int)syscall(SYS_move_mount, (from_dfd), (from_path), \
		      (to_dfd), (to_path), (flags)))
# endif
#endif

#ifndef fsopen
# ifdef SYS_fsopen
#  define fsopen(fs_name, flags) \
	((int)syscall(SYS_fsopen, (fs_name), (flags)))
# endif
#endif

#ifndef fsconfig
# ifdef SYS_fsconfig
#  define fsconfig(fsfd, cmd, key, value, aux) \
	((int)syscall(SYS_fsconfig, (fsfd), (cmd), (key), (value), (aux)))
# endif
#endif

#ifndef fsmount
# ifdef SYS_fsmount
#  define fsmount(fsfd, flags, attr_flags) \
	((int)syscall(SYS_fsmount, (fsfd), (flags), (attr_flags)))
# endif
#endif

#ifndef fspick
# ifdef SYS_fspick
#  define fspick(dfd, path, flags) \
	((int)syscall(SYS_fspick, (dfd), (path), (flags)))
# endif
#endif

#ifndef mount_setattr
# ifdef SYS_mount_setattr
#  define mount_setattr(dfd, path, flags, attr, size) \
	((int)syscall(SYS_mount_setattr, (dfd), (path), (flags), (attr), \
		      (size)))
# endif
#endif

#endif

#endif
