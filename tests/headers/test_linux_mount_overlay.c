#include <sys/mount.h>

int main(void)
{
#if defined(__linux__)
	int (*p_open_tree)(int, const char *, unsigned int) = open_tree;
	int (*p_move_mount)(int, const char *, int, const char *, unsigned int) =
		move_mount;
	int (*p_fsopen)(const char *, unsigned int) = fsopen;
	int (*p_fsconfig)(int, unsigned int, const char *, const void *, int) =
		fsconfig;
	int (*p_fsmount)(int, unsigned int, unsigned int) = fsmount;
	int (*p_fspick)(int, const char *, unsigned int) = fspick;
	int (*p_mount_setattr)(int, const char *, unsigned int,
			      struct mount_attr *, size_t) = mount_setattr;

#ifndef FSOPEN_CLOEXEC
#error "FSOPEN_CLOEXEC is required"
#endif
#ifndef FSMOUNT_CLOEXEC
#error "FSMOUNT_CLOEXEC is required"
#endif
#ifndef MOVE_MOUNT_F_EMPTY_PATH
#error "MOVE_MOUNT_F_EMPTY_PATH is required"
#endif

	if (0) {
		struct mount_attr attr = {0};
		int fd = p_fsopen("tmpfs", FSOPEN_CLOEXEC);
		int fsconfig_set_flag = FSCONFIG_SET_FLAG;
		int fsconfig_set_string = FSCONFIG_SET_STRING;

		(void)p_fsconfig(fd, fsconfig_set_flag, "ro", 0, 0);
		(void)p_fsconfig(fd, fsconfig_set_string, "source", "tmpfs", 0);
		fd = p_fsmount(fd, FSMOUNT_CLOEXEC, 0);
		(void)p_move_mount(fd, "", AT_FDCWD, "/tmp",
				   MOVE_MOUNT_F_EMPTY_PATH);
		(void)p_open_tree(AT_FDCWD, "/", OPEN_TREE_CLOEXEC);
		(void)p_fspick(AT_FDCWD, "/", FSPICK_CLOEXEC);
		(void)p_mount_setattr(AT_FDCWD, "/", 0, &attr, sizeof(attr));
	}
#endif

	return 0;
}
