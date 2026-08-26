#include <dirent.h>

#ifdef alphasort64
#undef alphasort64
#endif

#ifdef scandir64
#undef scandir64
#endif

static int (*sort_function)(const struct dirent **,
			    const struct dirent **) = alphasort64;
static int (*scan_function)(const char *, struct dirent ***,
			    int (*)(const struct dirent *),
			    int (*)(const struct dirent **,
				    const struct dirent **)) = scandir64;

int main(void)
{
	return sort_function == 0 || scan_function == 0;
}
