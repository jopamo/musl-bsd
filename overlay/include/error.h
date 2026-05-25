#ifndef MUSL_BSD_OVERLAY_ERROR_H
#define MUSL_BSD_OVERLAY_ERROR_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void (*error_print_progname)(void);
extern unsigned int error_message_count;
extern int error_one_per_line;

void error(int status, int errnum, const char *format, ...)
	__attribute__((format(printf, 3, 4)));
void error_at_line(int status, int errnum, const char *fname,
		   unsigned int lineno, const char *format, ...)
	__attribute__((format(printf, 5, 6)));

#ifdef __cplusplus
}
#endif

#endif
