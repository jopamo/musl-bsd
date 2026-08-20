#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern int __isoc99_fscanf(FILE* stream, const char* format, ...);
extern int __isoc99_sscanf(const char* input, const char* format, ...);

typedef int (*fscanf_function)(FILE* stream, const char* format, ...);
typedef int (*sscanf_function)(const char* input, const char* format, ...);

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "scanf ABI test failed at line %d\n", __LINE__); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int verify_provider(const char* internal_name, const char* public_name, const void* function) {
    Dl_info info;

    CHECK(dlsym(RTLD_DEFAULT, internal_name) == function);
    CHECK(dlsym(RTLD_DEFAULT, public_name) == function);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr(function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    return 0;
}

static int verify_fscanf_observed_formats(fscanf_function function) {
    static const char input[] =
        "DeviceFile: 42\n"
        "DeviceFileMinor: -7\n"
        "4294967295\n"
        "token ignored remainder\n"
        "Node 3 node0 123456 kB\n"
        "0x1.8p+2\n";
    char name[32];
    char node[32];
    char token[16];
    double hexadecimal;
    int signed_value;
    unsigned int unsigned_value;
    unsigned long kilobytes;
    FILE* stream;

    stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(fwrite(input, 1, sizeof(input) - 1, stream) == sizeof(input) - 1);
    rewind(stream);

    errno = EDOM;
    CHECK(function(stream, "%31[^:]: %u\n", name, &unsigned_value) == 2);
    CHECK(errno == EDOM);
    CHECK(strcmp(name, "DeviceFile") == 0);
    CHECK(unsigned_value == 42);

    errno = ERANGE;
    CHECK(function(stream, "%31[^:]: %d\n", name, &signed_value) == 2);
    CHECK(errno == ERANGE);
    CHECK(strcmp(name, "DeviceFileMinor") == 0);
    CHECK(signed_value == -7);

    errno = E2BIG;
    CHECK(function(stream, "%u", &unsigned_value) == 1);
    CHECK(errno == E2BIG);
    CHECK(unsigned_value == UINT_MAX);

    errno = ENOTTY;
    CHECK(function(stream, "%15s%*[^\n]\n", token) == 1);
    CHECK(errno == ENOTTY);
    CHECK(strcmp(token, "token") == 0);

    errno = ENOSPC;
    CHECK(function(stream, "Node %*d %s %lu kB\n", node, &kilobytes) == 2);
    CHECK(errno == ENOSPC);
    CHECK(strcmp(node, "node0") == 0);
    CHECK(kilobytes == 123456UL);

    errno = EBUSY;
    CHECK(function(stream, "%la\n", &hexadecimal) == 1);
    CHECK(errno == EBUSY);
    CHECK(hexadecimal == 6.0);
    CHECK(feof(stream));
    CHECK(!ferror(stream));

    CHECK(fclose(stream) == 0);
    return 0;
}

static int verify_fscanf_matching_and_input_failures(fscanf_function function) {
    unsigned int first;
    unsigned int second = 99;
    FILE* stream;

    stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(fwrite("word", 1, 4, stream) == 4);
    rewind(stream);
    errno = EDOM;
    CHECK(function(stream, "%u", &first) == 0);
    CHECK(errno == EINVAL);
    CHECK(fgetc(stream) == 'w');
    CHECK(!ferror(stream));
    CHECK(fclose(stream) == 0);

    stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(fwrite("7", 1, 1, stream) == 1);
    rewind(stream);
    errno = ERANGE;
    CHECK(function(stream, "%u %u", &first, &second) == 1);
    CHECK(errno == ERANGE);
    CHECK(first == 7);
    CHECK(second == 99);
    CHECK(feof(stream));
    CHECK(!ferror(stream));
    CHECK(fclose(stream) == 0);

    stream = tmpfile();
    CHECK(stream != NULL);
    errno = E2BIG;
    CHECK(function(stream, "%u", &first) == EOF);
    CHECK(errno == E2BIG);
    CHECK(feof(stream));
    CHECK(!ferror(stream));
    CHECK(fclose(stream) == 0);
    return 0;
}

static int verify_fscanf_stream_error(fscanf_function function) {
    unsigned int value;
    int pipe_fds[2];
    FILE* stream;

    CHECK(pipe(pipe_fds) == 0);
    CHECK(close(pipe_fds[0]) == 0);
    stream = fdopen(pipe_fds[1], "w");
    CHECK(stream != NULL);
    errno = EDOM;
    CHECK(function(stream, "%u", &value) == EOF);
    CHECK(errno == EDOM);
    CHECK(ferror(stream));
    CHECK(fclose(stream) == 0);
    return 0;
}

static int verify_sscanf_observed_formats(sscanf_function function) {
    char character = '\0';
    char text[64];
    char trailing = '\0';
    double hexadecimal_float;
    int first_signed;
    int second_signed;
    long long signed_long_long;
    unsigned int first_unsigned;
    unsigned int fourth_unsigned;
    unsigned int second_unsigned;
    unsigned int third_unsigned;
    unsigned long long unsigned_long_long;

    errno = EDOM;
    CHECK(function("1a:2b.3", "%x:%x.%x", &first_unsigned, &second_unsigned, &third_unsigned) == 3);
    CHECK(first_unsigned == 0x1a);
    CHECK(second_unsigned == 0x2b);
    CHECK(third_unsigned == 3);
    CHECK(function("1a:2b:3c.4d", "%x:%x:%x.%x", &first_unsigned, &second_unsigned, &third_unsigned,
                   &fourth_unsigned) == 4);
    CHECK(first_unsigned == 0x1a);
    CHECK(second_unsigned == 0x2b);
    CHECK(third_unsigned == 0x3c);
    CHECK(fourth_unsigned == 0x4d);
    CHECK(function("1a:2b.3!", "%x:%x.%x%c", &first_unsigned, &second_unsigned, &third_unsigned, &trailing) == 4);
    CHECK(trailing == '!');
    CHECK(function("1a:2b:3!", "%x:%x:%x%c", &first_unsigned, &second_unsigned, &third_unsigned, &trailing) == 4);
    CHECK(trailing == '!');
    CHECK(function("1a:2b:3c.4d!", "%x:%x:%x.%x%c", &first_unsigned, &second_unsigned, &third_unsigned,
                   &fourth_unsigned, &trailing) == 5);
    CHECK(fourth_unsigned == 0x4d);
    CHECK(trailing == '!');
    CHECK(function("0001:02:03.4", "%04x:%02x:%02x.%1u", &first_unsigned, &second_unsigned, &third_unsigned,
                   &fourth_unsigned) == 4);
    CHECK(first_unsigned == 1);
    CHECK(second_unsigned == 2);
    CHECK(third_unsigned == 3);
    CHECK(fourth_unsigned == 4);
    CHECK(errno == EDOM);

    errno = ERANGE;
    CHECK(function("channel12", "channel%d", &first_signed) == 1);
    CHECK(first_signed == 12);
    CHECK(function("in4_label", "in%d_label", &first_signed) == 1);
    CHECK(first_signed == 4);
    CHECK(function(".nv.constant7", ".nv.constant%d", &first_signed) == 1);
    CHECK(first_signed == 7);
    CHECK(function("Cuda compilation tools, release 13.3, V13.3", "Cuda compilation tools, release %d.%d,",
                   &first_signed, &second_signed) == 2);
    CHECK(first_signed == 13);
    CHECK(second_signed == 3);
    CHECK(errno == ERANGE);

    errno = E2BIG;
    CHECK(function("  -9 ignored", " %d %*s", &first_signed) == 1);
    CHECK(first_signed == -9);
    CHECK(function("prefix/12/34", "%*[^/]/%u/%u", &first_unsigned, &second_unsigned) == 2);
    CHECK(first_unsigned == 12);
    CHECK(second_unsigned == 34);
    CHECK(function("value,rest", "%[^,]", text) == 1);
    CHECK(strcmp(text, "value") == 0);
    CHECK(function("Z", "%c", &character) == 1);
    CHECK(character == 'Z');
    CHECK(errno == E2BIG);

    errno = ENOTTY;
    CHECK(function("-17", "%d", &first_signed) == 1);
    CHECK(first_signed == -17);
    CHECK(function("7.11", "%d.%d", &first_signed, &second_signed) == 2);
    CHECK(first_signed == 7);
    CHECK(second_signed == 11);
    CHECK(function("077", "%i", &first_signed) == 1);
    CHECK(first_signed == 63);
    CHECK(function("-0x2a", "%lli", &signed_long_long) == 1);
    CHECK(signed_long_long == -42);
    CHECK(function("feed", "%llx", &unsigned_long_long) == 1);
    CHECK(unsigned_long_long == 0xfeed);
    CHECK(function("42", "%u", &first_unsigned) == 1);
    CHECK(first_unsigned == 42);
    CHECK(function("17x", "%ux", &first_unsigned) == 1);
    CHECK(first_unsigned == 17);
    CHECK(function("f", "%1x", &first_unsigned) == 1);
    CHECK(first_unsigned == 15);
    CHECK(function("18446744073709551615", "%llu", &unsigned_long_long) == 1);
    CHECK(unsigned_long_long == ULLONG_MAX);
    CHECK(errno == ENOTTY);

    errno = ENOSPC;
    CHECK(function("0x1.8p+2", "%la", &hexadecimal_float) == 1);
    CHECK(hexadecimal_float == 6.0);
    CHECK(errno == ENOSPC);
    return 0;
}

static int verify_sscanf_failures(sscanf_function function) {
    unsigned int first = 77;
    unsigned int second = 99;

    errno = EDOM;
    CHECK(function("word", "%u", &first) == 0);
    CHECK(errno == EINVAL);
    CHECK(first == 77);

    errno = ERANGE;
    CHECK(function("7 word", "%u %u", &first, &second) == 1);
    CHECK(errno == EINVAL);
    CHECK(first == 7);
    CHECK(second == 99);

    errno = E2BIG;
    CHECK(function("", "%u", &first) == EOF);
    CHECK(errno == E2BIG);
    return 0;
}

int main(void) {
    fscanf_function fscanf_adapter = __isoc99_fscanf;
    sscanf_function sscanf_adapter = __isoc99_sscanf;

    _Static_assert(sizeof(int) == 4, "x86_64 glibc int ABI");
    _Static_assert(sizeof(long) == 8, "x86_64 glibc long ABI");
    _Static_assert(sizeof(long long) == 8, "x86_64 glibc long long ABI");
    CHECK(verify_provider("__isoc99_fscanf", "fscanf", (const void*)fscanf_adapter) == 0);
    CHECK(verify_provider("__isoc99_sscanf", "sscanf", (const void*)sscanf_adapter) == 0);

    CHECK(verify_fscanf_observed_formats(fscanf_adapter) == 0);
    CHECK(verify_fscanf_matching_and_input_failures(fscanf_adapter) == 0);
    CHECK(verify_fscanf_stream_error(fscanf_adapter) == 0);
    CHECK(verify_sscanf_observed_formats(sscanf_adapter) == 0);
    CHECK(verify_sscanf_failures(sscanf_adapter) == 0);
    return 0;
}
