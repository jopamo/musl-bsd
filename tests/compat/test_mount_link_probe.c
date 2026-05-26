#include <stddef.h>

struct mount_attr;

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

int main(void)
{
#if defined(__linux__)
	if (open_tree(-1, "", 0) != -1)
		return 1;
	if (move_mount(-1, "", -1, "", 0) != -1)
		return 1;
	if (fsopen("", 0) != -1)
		return 1;
	if (fsconfig(-1, 0, "", 0, 0) != -1)
		return 1;
	if (fsmount(-1, 0, 0) != -1)
		return 1;
	if (fspick(-1, "", 0) != -1)
		return 1;
	if (mount_setattr(-1, "", 0, 0, 0) != -1)
		return 1;

	return 0;
#else
	return 0;
#endif
}
