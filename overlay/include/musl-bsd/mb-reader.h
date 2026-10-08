#ifndef MUSL_BSD_MB_READER_H
#define MUSL_BSD_MB_READER_H

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <wchar.h>

struct musl_bsd_mb_reader {
    FILE* stream;
    mbstate_t state;
    unsigned char pending[MB_LEN_MAX];
    size_t pending_len;
};

/* A reader exclusively owns byte input from its stream. Keep its effective
   LC_CTYPE locale fixed and do not modify its fields or interleave reads/seeks. */
void musl_bsd_mb_reader_init(struct musl_bsd_mb_reader* reader, FILE* stream);
/* WEOF with EILSEQ consumes one malformed byte into invalid_byte; a null
   invalid_byte leaves all malformed input pending. Incomplete EOF is malformed.
   Stream errors leave partial input pending and preserve the I/O errno;
   call clearerr(stream) before retrying. Clean EOF leaves invalid_byte at -1. */
wint_t musl_bsd_mb_reader_next(struct musl_bsd_mb_reader* reader, int* invalid_byte);

#endif
