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

void musl_bsd_mb_reader_init(struct musl_bsd_mb_reader* reader, FILE* stream);
/* A null invalid_byte leaves malformed input pending. */
wint_t musl_bsd_mb_reader_next(struct musl_bsd_mb_reader* reader, int* invalid_byte);

#endif
