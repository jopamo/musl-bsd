#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern int _IO_putc(int byte, FILE* stream);

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "_IO_putc test failed at line %d\n", __LINE__); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

int main(void) {
    static const int inputs[] = {0x00, 0x7f, 0x80, 0xff, -1, 0x1ff};
    static const unsigned char expected[] = {0x00, 0x7f, 0x80, 0xff, 0xff, 0xff};
    int (*io_putc)(int, FILE*) = _IO_putc;
    unsigned char output[sizeof(expected)];
    int pipe_fds[2];
    Dl_info info;
    FILE* stream;

    stream = tmpfile();
    CHECK(stream != NULL);
    for (size_t index = 0; index < sizeof(inputs) / sizeof(inputs[0]); ++index) {
        errno = EDOM;
        CHECK(io_putc(inputs[index], stream) == expected[index]);
        CHECK(errno == EDOM);
    }
    CHECK(fflush(stream) == 0);
    rewind(stream);
    CHECK(fread(output, 1, sizeof(output), stream) == sizeof(output));
    CHECK(memcmp(output, expected, sizeof(expected)) == 0);
    CHECK(fclose(stream) == 0);

    CHECK(pipe(pipe_fds) == 0);
    CHECK(close(pipe_fds[1]) == 0);
    stream = fdopen(pipe_fds[0], "r");
    CHECK(stream != NULL);
    errno = ERANGE;
    CHECK(io_putc('x', stream) == EOF);
    CHECK(ferror(stream));
    CHECK(errno == ERANGE);
    CHECK(fclose(stream) == 0);

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)io_putc, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "_IO_putc") == (void*)io_putc);
    return 0;
}
