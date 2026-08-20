#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern int _IO_getc(FILE* stream);

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "_IO_getc test failed at line %d\n", __LINE__); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

int main(void) {
    static const unsigned char bytes[] = {0x00, 0x7f, 0x80, 0xff, '\n'};
    int (*io_getc)(FILE*) = _IO_getc;
    int pipe_fds[2];
    Dl_info info;
    FILE* stream;

    stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(fwrite(bytes, 1, sizeof(bytes), stream) == sizeof(bytes));
    rewind(stream);

    for (size_t index = 0; index < sizeof(bytes); ++index) {
        errno = EDOM;
        CHECK(io_getc(stream) == bytes[index]);
        CHECK(errno == EDOM);
    }
    errno = ERANGE;
    CHECK(io_getc(stream) == EOF);
    CHECK(feof(stream));
    CHECK(!ferror(stream));
    CHECK(errno == ERANGE);

    clearerr(stream);
    CHECK(ungetc(0xa5, stream) == 0xa5);
    CHECK(io_getc(stream) == 0xa5);
    CHECK(fclose(stream) == 0);

    CHECK(pipe(pipe_fds) == 0);
    CHECK(close(pipe_fds[0]) == 0);
    stream = fdopen(pipe_fds[1], "w");
    CHECK(stream != NULL);
    errno = EDOM;
    CHECK(io_getc(stream) == EOF);
    CHECK(ferror(stream));
    CHECK(errno == EDOM);
    CHECK(fclose(stream) == 0);

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)io_getc, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "_IO_getc") == (void*)io_getc);
    return 0;
}
