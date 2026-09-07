#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <musl-bsd/readpassphrase.h>

int __real_tcsetattr(int, int, const struct termios *);

int
__wrap_tcsetattr(int fd, int action, const struct termios *term)
{
    if (getenv("FAIL_TCSETATTR")) {
        errno = EIO;
        return -1;
    }
    return __real_tcsetattr(fd, action, term);
}

int
main(int argc, char **argv)
{
    char buf[16];
    char *result;
    int flags, error;
    size_t size;

    if (argc != 3)
        return 2;
    flags = atoi(argv[1]);
    size = (size_t)atoi(argv[2]);
    if (size > sizeof(buf))
        return 2;
    if (getenv("CLOSE_STDIN"))
        close(STDIN_FILENO);
    memset(buf, 'x', sizeof(buf));
    errno = 0;
    result = readpassphrase("secret: ", buf, size, flags);
    error = errno;
    if (!result) {
        printf("error:%d\n", error);
        return 0;
    }
    printf("result:%s\n", result);
    return 0;
}
