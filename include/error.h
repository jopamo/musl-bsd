#ifndef _ERROR_H_
#define _ERROR_H_

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(program_invocation_name)
static const char* global_program_name = "unknown";

static inline void set_program_invocation_name(const char* name) {
    global_program_name = name ? name : "unknown";
}

#define program_invocation_name (global_program_name)
#endif

static unsigned int error_message_count = 0;

static inline void error(int status, int errnum, const char* format, ...) {
    va_list ap;

    fprintf(stderr, "%s: ", program_invocation_name);

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    if (errnum) {
        fprintf(stderr, ": %s", strerror(errnum));
    }

    fputc('\n', stderr);

    error_message_count++;

    if (status) {
        exit(status);
    }
}

#endif
