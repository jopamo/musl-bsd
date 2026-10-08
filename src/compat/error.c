#include <error.h>

#include <errno.h>
#include <pthread.h>
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
	static char *last_fname;
	static unsigned int last_lineno;
	static int have_last;
	int cancel_state;

	(void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel_state);
	fflush(stdout);
	flockfile(stderr);
	if (error_one_per_line && with_location) {
		int same_file = have_last && (fname == last_fname ||
		    (fname != NULL && last_fname != NULL && strcmp(last_fname, fname) == 0));
		if (same_file && last_lineno == lineno)
			goto done;
		if (!same_file) {
			char *copy = fname != NULL ? strdup(fname) : NULL;
			free(last_fname);
			last_fname = copy;
			/* An allocation failure drops suppression, not the diagnostic. */
			have_last = fname == NULL || copy != NULL;
		}
		last_lineno = lineno;
	}

	if (error_print_progname != NULL) {
		error_print_progname();
	} else if (program_invocation_name != NULL &&
		   program_invocation_name[0] != '\0') {
		fprintf(stderr, "%s:%s", program_invocation_name, with_location ? "" : " ");
	}

	if (with_location) {
		if (fname != NULL)
			fprintf(stderr, "%s:%u: ", fname, lineno);
		else
			fputc(' ', stderr);
	}

	vfprintf(stderr, format, ap);

	if (errnum != 0)
		fprintf(stderr, ": %s", strerror(errnum));

	fputc('\n', stderr);
	error_message_count++;
	fflush(stderr);

	if (status != 0)
		exit(status);
done:
	funlockfile(stderr);
	(void)pthread_setcancelstate(cancel_state, NULL);
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
