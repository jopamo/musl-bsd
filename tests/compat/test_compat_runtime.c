#include <error.h>
#include <fnmatch.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>

int main(void)
{
	char buf[8];

	if (strlcpy(buf, "abc", sizeof(buf)) != 3)
		return 1;
	if (strcmp(buf, "abc") != 0)
		return 1;

	if (strlcat(buf, "defghi", sizeof(buf)) != 9)
		return 1;
	if (strcmp(buf, "abcdefg") != 0)
		return 1;

	if (TEMP_FAILURE_RETRY(0) != 0)
		return 1;

	if (W_EXITCODE(5, 9) != ((5 << 8) | 9))
		return 1;

	if (ACCESSPERMS != 0777)
		return 1;
	if (ALLPERMS != 07777)
		return 1;
	if (DEFFILEMODE != 0666)
		return 1;
	if (FNM_EXTMATCH != 0)
		return 1;

	if (freopen("/dev/null", "w", stderr) == NULL)
		return 1;

	error_message_count = 0;
	error_one_per_line = 1;

	error_at_line(0, EINVAL, "sample.c", 42, "badness");
	error_at_line(0, EINVAL, "sample.c", 42, "badness");

	if (error_message_count != 1)
		return 1;

	error_one_per_line = 0;
	error(0, 0, "plain message");

	if (error_message_count != 2)
		return 1;

	return 0;
}
