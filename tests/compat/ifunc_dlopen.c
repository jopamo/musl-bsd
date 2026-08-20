#include <dlfcn.h>

int main(int argc, char **argv) {
	void *handle;
	int (**function)(void);
	const char *error;

	if (argc != 2)
		return 2;
	handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (handle == 0)
		return 3;
	function = (int (**)(void))dlsym(handle, "ifunc_a_pointer");
	error = dlerror();
	if (error != 0 || function == 0 || *function == 0)
		return 4;
	if ((*function)() != 41)
		return 5;
	return dlclose(handle) == 0 ? 0 : 6;
}
