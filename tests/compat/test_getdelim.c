#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

extern ssize_t __getdelim(char** line, size_t* capacity, int delimiter, FILE* stream);

typedef ssize_t (*getdelim_function)(char** line, size_t* capacity, int delimiter, FILE* stream);

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "__getdelim test failed at line %d\n", __LINE__); \
            return 1;                                                         \
        }                                                                     \
    } while (0)

static int verify_line_semantics(getdelim_function function) {
    static const unsigned char input[] = {
        'a', 'l', 'p', 'h', 'a', '\n', 'b', 'e', 't', 'a', '\0', 't', 'a', 'i', 'l', '|', 'o', 'm', 'e', 'g', 'a',
    };
    static const unsigned char second_record[] = {
        'b', 'e', 't', 'a', '\0', 't', 'a', 'i', 'l', '|',
    };
    char* line = NULL;
    size_t capacity = 0;
    FILE* stream;
    ssize_t length;

    stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(fwrite(input, 1, sizeof(input), stream) == sizeof(input));
    rewind(stream);

    errno = EDOM;
    length = function(&line, &capacity, '\n', stream);
    CHECK(length == 6);
    CHECK(errno == EDOM);
    CHECK(memcmp(line, "alpha\n", 6) == 0);
    CHECK(line[6] == '\0');
    CHECK(capacity >= 7);

    errno = ERANGE;
    length = function(&line, &capacity, 0x100 | '|', stream);
    CHECK(length == (ssize_t)sizeof(second_record));
    CHECK(errno == ERANGE);
    CHECK(memcmp(line, second_record, sizeof(second_record)) == 0);
    CHECK(line[sizeof(second_record)] == '\0');

    errno = E2BIG;
    length = function(&line, &capacity, '\n', stream);
    CHECK(length == 5);
    CHECK(errno == E2BIG);
    CHECK(memcmp(line, "omega", 5) == 0);
    CHECK(line[5] == '\0');
    CHECK(feof(stream));
    CHECK(!ferror(stream));

    errno = ENOTTY;
    CHECK(function(&line, &capacity, '\n', stream) == -1);
    CHECK(errno == ENOTTY);
    CHECK(feof(stream));
    CHECK(!ferror(stream));

    free(line);
    CHECK(fclose(stream) == 0);
    return 0;
}

static int verify_growth_and_reuse(getdelim_function function) {
    enum {
        payload_size = 8192,
    };
    unsigned char input[payload_size + 1];
    char* line;
    char* reused_line;
    size_t capacity = 4;
    size_t reused_capacity;
    FILE* stream;

    memset(input, 'x', payload_size);
    input[payload_size] = '\n';
    line = malloc(capacity);
    CHECK(line != NULL);
    stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(fwrite(input, 1, sizeof(input), stream) == sizeof(input));
    CHECK(fwrite("end\n", 1, 4, stream) == 4);
    rewind(stream);

    errno = EDOM;
    CHECK(function(&line, &capacity, '\n', stream) == (ssize_t)sizeof(input));
    CHECK(errno == EDOM);
    CHECK(capacity > sizeof(input));
    CHECK(memcmp(line, input, sizeof(input)) == 0);
    CHECK(line[sizeof(input)] == '\0');

    reused_line = line;
    reused_capacity = capacity;
    errno = ERANGE;
    CHECK(function(&line, &capacity, '\n', stream) == 4);
    CHECK(errno == ERANGE);
    CHECK(line == reused_line);
    CHECK(capacity == reused_capacity);
    CHECK(memcmp(line, "end\n", 4) == 0);
    CHECK(line[4] == '\0');

    free(line);
    CHECK(fclose(stream) == 0);
    return 0;
}

static int verify_failures(getdelim_function function) {
    char* line = NULL;
    size_t capacity = 0;
    int pipe_fds[2];
    FILE* stream;

    stream = tmpfile();
    CHECK(stream != NULL);
    errno = 0;
    CHECK(function(NULL, &capacity, '\n', stream) == -1);
    CHECK(errno == EINVAL);
    CHECK(ferror(stream));

    clearerr(stream);
    errno = 0;
    CHECK(function(&line, NULL, '\n', stream) == -1);
    CHECK(errno == EINVAL);
    CHECK(ferror(stream));
    CHECK(fclose(stream) == 0);

    CHECK(pipe(pipe_fds) == 0);
    CHECK(close(pipe_fds[0]) == 0);
    stream = fdopen(pipe_fds[1], "w");
    CHECK(stream != NULL);
    errno = EDOM;
    CHECK(function(&line, &capacity, '\n', stream) == -1);
    CHECK(errno == EDOM);
    CHECK(ferror(stream));
    free(line);
    CHECK(fclose(stream) == 0);
    return 0;
}

int main(void) {
    getdelim_function function = __getdelim;
    Dl_info info;

    CHECK(sizeof(ssize_t) == sizeof(void*));
    CHECK((ssize_t)-1 < 0);
    CHECK(dlsym(RTLD_DEFAULT, "__getdelim") == (void*)function);
    CHECK(dlsym(RTLD_DEFAULT, "getdelim") == (void*)function);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);

    CHECK(verify_line_semantics(function) == 0);
    CHECK(verify_growth_and_reuse(function) == 0);
    CHECK(verify_failures(function) == 0);
    return 0;
}
