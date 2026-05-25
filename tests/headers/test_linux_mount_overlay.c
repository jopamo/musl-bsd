#include <sys/mount.h>

int main(void)
{
#if defined(__linux__)
#ifndef FSOPEN_CLOEXEC
#error "FSOPEN_CLOEXEC is required"
#endif
#ifndef FSMOUNT_CLOEXEC
#error "FSMOUNT_CLOEXEC is required"
#endif
#ifndef MOVE_MOUNT_F_EMPTY_PATH
#error "MOVE_MOUNT_F_EMPTY_PATH is required"
#endif

#ifndef open_tree
#error "open_tree shim is required"
#endif
#ifndef move_mount
#error "move_mount shim is required"
#endif
#ifndef fsopen
#error "fsopen shim is required"
#endif
#ifndef fsconfig
#error "fsconfig shim is required"
#endif
#ifndef fsmount
#error "fsmount shim is required"
#endif
#ifndef fspick
#error "fspick shim is required"
#endif
#ifndef mount_setattr
#error "mount_setattr shim is required"
#endif

	if (0) {
		struct mount_attr attr = {0};
		int fd = fsopen("tmpfs", FSOPEN_CLOEXEC);
		int fsconfig_set_flag = FSCONFIG_SET_FLAG;
		int fsconfig_set_string = FSCONFIG_SET_STRING;

		(void)fsconfig(fd, fsconfig_set_flag, "ro", 0, 0);
		(void)fsconfig(fd, fsconfig_set_string, "source", "tmpfs", 0);
		fd = fsmount(fd, FSMOUNT_CLOEXEC, 0);
		(void)move_mount(fd, "", AT_FDCWD, "/tmp",
				 MOVE_MOUNT_F_EMPTY_PATH);
		(void)open_tree(AT_FDCWD, "/", OPEN_TREE_CLOEXEC);
		(void)fspick(AT_FDCWD, "/", FSPICK_CLOEXEC);
		(void)mount_setattr(AT_FDCWD, "/", 0, &attr, sizeof(attr));
	}
#endif

	return 0;
}
