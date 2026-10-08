/*	$OpenBSD: readpassphrase.c,v 1.26 2016/10/18 12:47:18 millert Exp $	*/

/*
 * Copyright (c) 2000-2002, 2007, 2010
 *	Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F39502-99-1-0512.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <musl-bsd/readpassphrase.h>

#ifndef TCSASOFT
#define TCSASOFT 0
#endif

#ifndef _NSIG
#if defined(NSIG)
#define _NSIG NSIG
#else
/* The SIGRTMAX define might be set to a function such as sysconf(). */
#define _NSIG (SIGRTMAX + 1)
#endif
#endif

static volatile sig_atomic_t signo[_NSIG];
static const int caught_signals[] = {
	SIGALRM, SIGHUP, SIGINT, SIGPIPE, SIGQUIT, SIGTERM,
	SIGTSTP, SIGTTIN, SIGTTOU
};

#define NUM_SIGNALS (sizeof(caught_signals) / sizeof(caught_signals[0]))

struct cleanup_state {
	int input, tty_opened, term_changed, mask_changed, error, restore_error;
	size_t installed;
	unsigned char owned[NUM_SIGNALS], restored[_NSIG];
	struct termios oterm;
	struct sigaction saved[NUM_SIGNALS];
	sigset_t mask, blocked;
	char *buf;
	size_t bufsiz;
};

static void handler(int);

static void
record_error(struct cleanup_state *state, int error)
{
	if (!state->error)
		state->error = error ? error : EIO;
}

static int
restore_mask(struct cleanup_state *state)
{
	if (state->mask_changed) {
		/* Allow the saved job-control mask during terminal restoration. */
		int error = pthread_sigmask(SIG_SETMASK, &state->mask, NULL);
		if (error) {
			record_error(state, error);
			state->restore_error = 1;
			return -1;
		} else {
			state->mask_changed = 0;
		}
	}
	return 0;
}

/* Called with cancellation disabled, including during cancellation cleanup. */
static void
restore_state(struct cleanup_state *state)
{
	(void)restore_mask(state);
	if (state->term_changed) {
		const int sigttou = signo[SIGTTOU];
		int result;

		/* Ignore SIGTTOU generated when we are not the fg pgrp. */
		do {
			result = tcsetattr(state->input, TCSAFLUSH|TCSASOFT, &state->oterm);
		} while (result == -1 && errno == EINTR && !signo[SIGTTOU]);
		if (result == -1) {
			record_error(state, errno);
			state->restore_error = 1;
		}
		signo[SIGTTOU] = sigttou;
		state->term_changed = 0;
	}
	for (size_t action = 0; action < state->installed; action++) {
		int sig = caught_signals[action];
		if (!state->owned[action])
			continue;
		if (sigaction(sig, &state->saved[action], NULL) == -1) {
			record_error(state, errno);
			state->restore_error = 1;
		} else {
			state->owned[action] = 0;
			state->restored[sig] = 1;
		}
	}
	if (state->tty_opened) {
		state->tty_opened = 0;
		if (close(state->input) == -1) {
			record_error(state, errno);
			state->restore_error = 1;
		}
	}
}

static int
redeliver_signals(struct cleanup_state *state)
{
	int need_restart = 0, wiped = 0;
	for (int i = 0; i < _NSIG; i++) {
		if (signo[i] && state->restored[i]) {
			record_error(state, EINTR);
			if (!wiped) {
				explicit_bzero(state->buf, state->bufsiz);
				wiped = 1;
			}
			signo[i] = 0;
			if (kill(getpid(), i) == -1) {
				record_error(state, errno);
				state->restore_error = 1;
				continue;
			}
			switch (i) {
			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				need_restart = 1;
			}
		}
	}
	return need_restart;
}

static void
cancel_read(void *argument)
{
	struct cleanup_state *state = argument;
	(void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	explicit_bzero(state->buf, state->bufsiz);
	restore_state(state);
	(void)redeliver_signals(state);
}

static int
write_output(int fd, const char *data, size_t length)
{
	while (length) {
		ssize_t count = write(fd, data, length);
		if (count <= 0) {
			if (count == 0)
				errno = EIO;
			return -1;
		}
		data += count;
		length -= (size_t)count;
	}
	return 0;
}

/* Block signals before checking flags; ppoll atomically restores the mask.
   Input must have no competing readers or concurrent descriptor changes. */
static ssize_t
read_byte(struct cleanup_state *state, char *byte)
{
	struct pollfd input = { .fd = state->input, .events = POLLIN };

	for (;;) {
		for (size_t action = 0; action < NUM_SIGNALS; action++) {
			if (signo[caught_signals[action]]) {
				errno = EINTR;
				return -1;
			}
		}
		if (ppoll(&input, 1, NULL, &state->mask) == -1)
			return -1;
		/* Ready input is exclusive; retain SIGTTIN behavior during the read. */
		int error = pthread_sigmask(SIG_SETMASK, &state->mask, NULL);
		if (error) {
			errno = error;
			return -1;
		}
		ssize_t count = read(state->input, byte, 1);
		int read_error = errno;
		error = pthread_sigmask(SIG_BLOCK, &state->blocked, NULL);
		if (error) {
			if (count == -1)
				record_error(state, read_error);
			record_error(state, error);
			state->restore_error = 1;
			if (count != -1)
				read_error = error;
			count = -1;
		}
		errno = read_error;
		if (count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
			continue;
		return count;
	}
}

static char *
read_passphrase(const char *prompt, struct cleanup_state *state, int flags,
    int cancel_state)
{
	ssize_t nr;
	int output, save_errno, setup_done;
	char ch, *p, *end;
	struct termios term;
	struct sigaction sa;

restart:
	for (int i = 0; i < _NSIG; i++)
		signo[i] = 0;
	save_errno = 0;
	setup_done = 0;
	state->input = output = -1;
	state->tty_opened = state->term_changed = state->mask_changed = 0;
	state->error = state->restore_error = 0;
	state->installed = 0;
	memset(state->owned, 0, sizeof(state->owned));
	memset(state->restored, 0, sizeof(state->restored));
	/* Read/write /dev/tty, or borrow stdin/stderr unless a tty is required. */
	if (!(flags & RPP_STDIN))
		state->input = output = open(_PATH_TTY, O_RDWR | O_CLOEXEC);
	state->tty_opened = state->input != -1;
	if (!state->tty_opened) {
		if (flags & RPP_REQUIRE_TTY) {
			record_error(state, ENOTTY);
			goto restore;
		}
		state->input = STDIN_FILENO;
		output = STDERR_FILENO;
	}

	/* A background pgrp must receive SIGTTOU before we install handlers. */
	if (state->tty_opened) {
		if (tcgetattr(state->input, &state->oterm) == -1) {
			record_error(state, errno);
			goto restore;
		}
		memcpy(&term, &state->oterm, sizeof(term));
		if (!(flags & RPP_ECHO_ON))
			term.c_lflag &= ~(ECHO | ECHONL);
#ifdef VSTATUS
		if (term.c_cc[VSTATUS] != _POSIX_VDISABLE)
			term.c_cc[VSTATUS] = _POSIX_VDISABLE;
#endif
		/* Never prompt for a secret if terminal setup failed. */
		if (tcsetattr(state->input, TCSAFLUSH|TCSASOFT, &term) == -1) {
			record_error(state, errno);
			goto restore;
		}
		state->term_changed = memcmp(&term, &state->oterm, sizeof(term)) != 0;
	} else {
		memset(&term, 0, sizeof(term));
		term.c_lflag = ECHO;
	}

	/* Do not restart reads interrupted by a caught signal. */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	while (state->installed < NUM_SIGNALS) {
		size_t action = state->installed;
		if (sigaction(caught_signals[action], &sa, &state->saved[action]) == -1) {
			record_error(state, errno);
			goto restore;
		}
		state->owned[action] = 1;
		state->installed++;
	}
	setup_done = 1;

	(void)pthread_setcancelstate(cancel_state, NULL);
	if (!(flags & RPP_STDIN) && write_output(output, prompt, strlen(prompt)) == -1) {
		record_error(state, errno);
		goto restore;
	}
	sigemptyset(&state->blocked);
	for (size_t action = 0; action < NUM_SIGNALS; action++)
		sigaddset(&state->blocked, caught_signals[action]);
	int mask_error = pthread_sigmask(SIG_BLOCK, &state->blocked, &state->mask);
	if (mask_error) {
		record_error(state, mask_error);
		goto restore;
	}
	state->mask_changed = 1;
	end = state->buf + state->bufsiz - 1;
	p = state->buf;
	while ((nr = read_byte(state, &ch)) == 1 && ch != '\n' && ch != '\r') {
		if (p < end) {
			if (flags & RPP_SEVENBIT)
				ch &= 0x7f;
			if (isalpha((unsigned char)ch)) {
				if (flags & RPP_FORCELOWER)
					ch = (char)tolower((unsigned char)ch);
				if (flags & RPP_FORCEUPPER)
					ch = (char)toupper((unsigned char)ch);
			}
			*p++ = ch;
		}
	}
	*p = '\0';
	save_errno = errno;
	if (nr == -1)
		record_error(state, save_errno);
	/* Retain SIGTTOU behavior for the final write as well as restoration. */
	if (restore_mask(state) == 0 && !(term.c_lflag & ECHO) && write_output(output, "\n", 1) == -1)
		record_error(state, errno);

restore:
	(void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	restore_state(state);
	if (redeliver_signals(state) && setup_done && !state->restore_error)
		goto restart;
	if (state->error) {
		explicit_bzero(state->buf, state->bufsiz);
		errno = state->error;
		return NULL;
	}
	errno = save_errno;
	return state->buf;
}

char *
readpassphrase(const char *prompt, char *buf, size_t bufsiz, int flags)
{
	struct cleanup_state state = { .buf = buf, .bufsiz = bufsiz };
	int old_state, old_type, save_errno;
	char *result;

	if (bufsiz == 0) {
		errno = EINVAL;
		return NULL;
	}
	(void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_state);
	(void)pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type);
	buf[0] = '\0';
	pthread_cleanup_push(cancel_read, &state);
	result = read_passphrase(prompt, &state, flags, old_state);
	save_errno = errno;
	/* Keep wiping armed while restoring the caller's cancellation mode. */
	(void)pthread_setcanceltype(old_type, NULL);
	(void)pthread_setcancelstate(old_state, NULL);
	pthread_testcancel();
	pthread_cleanup_pop(0);
	errno = save_errno;
	return result;
}

static void
handler(int s)
{
	signo[s] = 1;
}
