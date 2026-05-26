#include <sys/mount.h>

#ifdef __linux__

#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int open_tree(int dfd, const char *path, unsigned int flags)
{
#ifdef SYS_open_tree
	return (int) syscall(SYS_open_tree, dfd, path, flags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int move_mount(int from_dfd, const char *from_path, int to_dfd,
	       const char *to_path, unsigned int flags)
{
#ifdef SYS_move_mount
	return (int) syscall(SYS_move_mount, from_dfd, from_path, to_dfd,
			     to_path, flags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int fsopen(const char *fs_name, unsigned int flags)
{
#ifdef SYS_fsopen
	return (int) syscall(SYS_fsopen, fs_name, flags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int fsconfig(int fsfd, unsigned int cmd, const char *key,
	     const void *value, int aux)
{
#ifdef SYS_fsconfig
	return (int) syscall(SYS_fsconfig, fsfd, cmd, key, value, aux);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int fsmount(int fsfd, unsigned int flags, unsigned int attr_flags)
{
#ifdef SYS_fsmount
	return (int) syscall(SYS_fsmount, fsfd, flags, attr_flags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int fspick(int dfd, const char *path, unsigned int flags)
{
#ifdef SYS_fspick
	return (int) syscall(SYS_fspick, dfd, path, flags);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int mount_setattr(int dfd, const char *path, unsigned int flags,
		  struct mount_attr *attr, size_t size)
{
#ifdef SYS_mount_setattr
	return (int) syscall(SYS_mount_setattr, dfd, path, flags, attr, size);
#else
	errno = ENOSYS;
	return -1;
#endif
}

#endif
