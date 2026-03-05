#include "argp-fmtstream.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE* open_capture(char** out_buf, size_t* out_len) {
    *out_buf = NULL;
    *out_len = 0;
    FILE* stream = open_memstream(out_buf, out_len);
    assert(stream != NULL);
    return stream;
}

static void assert_lines_within(const char* s, size_t max_width) {
    const char* p = s;
    while (*p) {
        const char* nl = strchr(p, '\n');
        size_t line_len = nl ? (size_t)(nl - p) : strlen(p);
        assert(line_len <= max_width);
        if (!nl)
            break;
        p = nl + 1;
    }
}

static void test_truncate_mode(void) {
    char* out = NULL;
    size_t out_len = 0;
    FILE* stream = open_capture(&out, &out_len);
    argp_fmtstream_t fs = argp_make_fmtstream(stream, 0, 10, -1);
    assert(fs != NULL);

    assert(argp_fmtstream_puts(fs, "123456789012345") == 0);
    argp_fmtstream_free(fs);
    assert(fclose(stream) == 0);

    assert(strcmp(out, "123456789") == 0);
    free(out);
}

static void test_wrap_mode_with_margins(void) {
    char* out = NULL;
    size_t out_len = 0;
    FILE* stream = open_capture(&out, &out_len);
    argp_fmtstream_t fs = argp_make_fmtstream(stream, 2, 14, 4);
    assert(fs != NULL);

    assert(argp_fmtstream_puts(fs, "alpha beta gamma\n") == 0);
    argp_fmtstream_free(fs);
    assert(fclose(stream) == 0);

    assert(strstr(out, "alpha") != NULL);
    assert(strstr(out, "beta") != NULL);
    assert(strstr(out, "gamma") != NULL);
    assert(strstr(out, "\n    ") != NULL);
    assert_lines_within(out, 13);
    free(out);
}

static void test_long_word_wrap_path(void) {
    char* out = NULL;
    size_t out_len = 0;
    FILE* stream = open_capture(&out, &out_len);
    argp_fmtstream_t fs = argp_make_fmtstream(stream, 0, 12, 4);
    assert(fs != NULL);

    assert(argp_fmtstream_puts(fs, "supercalifragilistic exp\n") == 0);
    argp_fmtstream_free(fs);
    assert(fclose(stream) == 0);

    assert(strstr(out, "supercalifragilistic\n") != NULL);
    assert(strstr(out, "\n    exp\n") != NULL);
    free(out);
}

static void test_printf_growth_and_empty_puts(void) {
    char* out = NULL;
    size_t out_len = 0;
    FILE* stream = open_capture(&out, &out_len);
    argp_fmtstream_t fs = argp_make_fmtstream(stream, 0, 1024, 0);
    assert(fs != NULL);

    char big[701];
    memset(big, 'x', sizeof(big) - 1);
    big[sizeof(big) - 1] = '\0';

    assert(argp_fmtstream_puts(fs, "") == 0);
    assert(argp_fmtstream_printf(fs, "%s", big) == (ssize_t)strlen(big));
    assert(argp_fmtstream_putc(fs, '\n') == '\n');
    argp_fmtstream_free(fs);
    assert(fclose(stream) == 0);

    assert(out_len == strlen(big) + 1);
    assert(strncmp(out, big, strlen(big)) == 0);
    assert(out[strlen(big)] == '\n');
    free(out);
}

static void test_flush_failure_paths(void) {
    FILE* stream = fopen("/dev/full", "w");
    if (stream == NULL) {
        return;
    }
    assert(setvbuf(stream, NULL, _IONBF, 0) == 0);

    argp_fmtstream_t fs = argp_make_fmtstream(stream, 0, 256, 0);
    assert(fs != NULL);

    char prefix[181];
    memset(prefix, 'p', sizeof(prefix) - 1);
    prefix[sizeof(prefix) - 1] = '\0';
    assert(argp_fmtstream_puts(fs, prefix) == 0);

    char extra[64];
    memset(extra, 'q', sizeof(extra));
    errno = 0;
    assert(argp_fmtstream_write(fs, extra, sizeof(extra)) == 0);
    assert(errno != 0);

    errno = 0;
    assert(argp_fmtstream_printf(fs, "x") == -1);
    assert(errno != 0);

    argp_fmtstream_free(fs);
    assert(fclose(stream) == 0);
}

int main(void) {
    test_truncate_mode();
    test_wrap_mode_with_margins();
    test_long_word_wrap_path();
    test_printf_growth_and_empty_puts();
    test_flush_failure_paths();
    puts("test_fmtstream_behavior ok");
    return 0;
}
