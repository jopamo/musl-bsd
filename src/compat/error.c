#include <error.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void (*error_print_progname)(void);
unsigned int error_message_count;
int error_one_per_line;

static void error_messagev(int status, int errnum, const char *fname,
			   unsigned int lineno, const char *format,
			   va_list ap, int with_location)
{
	static const char *last_fname;
	static unsigned int last_lineno;

	if (error_one_per_line && with_location && last_fname != NULL && fname != NULL &&
	    last_lineno == lineno && strcmp(last_fname, fname) == 0)
		return;

	if (error_print_progname != NULL) {
		error_print_progname();
	} else if (program_invocation_name != NULL &&
		   program_invocation_name[0] != '\0') {
		fprintf(stderr, "%s: ", program_invocation_name);
	}

	if (with_location && fname != NULL)
		fprintf(stderr, "%s:%u: ", fname, lineno);

	vfprintf(stderr, format, ap);

	if (errnum != 0)
		fprintf(stderr, ": %s", strerror(errnum));

	fputc('\n', stderr);
	error_message_count++;

	if (with_location && fname != NULL) {
		last_fname = fname;
		last_lineno = lineno;
	}

	if (status != 0)
		exit(status);
}

void error(int status, int errnum, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	error_messagev(status, errnum, NULL, 0, format, ap, 0);
	va_end(ap);
}

void error_at_line(int status, int errnum, const char *fname,
		   unsigned int lineno, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	error_messagev(status, errnum, fname, lineno, format, ap, 1);
	va_end(ap);
}
