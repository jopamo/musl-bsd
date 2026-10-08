#include <musl-bsd/mb-reader.h>

#include <errno.h>
#include <string.h>

void musl_bsd_mb_reader_init(struct musl_bsd_mb_reader* reader, FILE* stream) {
    memset(reader, 0, sizeof(*reader));
    reader->stream = stream;
}

static int next_byte(struct musl_bsd_mb_reader* reader) {
    int byte;

    if (reader->pending_len == 0)
        return fgetc(reader->stream);

    byte = reader->pending[0];
    reader->pending_len--;
    memmove(reader->pending, reader->pending + 1, reader->pending_len);
    return byte;
}

static void replay_sequence(struct musl_bsd_mb_reader* reader, const unsigned char* sequence, size_t length) {
    /* The sequence plus remaining pending bytes is bounded by MB_LEN_MAX. */
    if (length != 0) {
        memmove(reader->pending + length, reader->pending, reader->pending_len);
        memcpy(reader->pending, sequence, length);
        reader->pending_len += length;
    }
}

static wint_t reject_sequence(struct musl_bsd_mb_reader* reader,
                              const unsigned char* sequence,
                              size_t length,
                              int* invalid_byte) {
    size_t consumed = invalid_byte != NULL ? 1 : 0;
    replay_sequence(reader, sequence + consumed, length - consumed);
    memset(&reader->state, 0, sizeof(reader->state));
    if (invalid_byte != NULL)
        *invalid_byte = sequence[0];
    errno = EILSEQ;
    return WEOF;
}

wint_t musl_bsd_mb_reader_next(struct musl_bsd_mb_reader* reader, int* invalid_byte) {
    unsigned char sequence[MB_LEN_MAX];
    mbstate_t initial_state = reader->state;
    size_t length = 0;
    wchar_t character;

    if (invalid_byte != NULL)
        *invalid_byte = -1;

    for (;;) {
        size_t result;
        int byte;

        if (length == sizeof(sequence))
            return reject_sequence(reader, sequence, length, invalid_byte);

        byte = next_byte(reader);
        if (byte == EOF) {
            if (length != 0) {
                int error = errno;
                if (!ferror(reader->stream))
                    return reject_sequence(reader, sequence, length, invalid_byte);
                replay_sequence(reader, sequence, length);
                reader->state = initial_state;
                errno = error;
            }
            return WEOF;
        }

        sequence[length++] = (unsigned char)byte;
        result = mbrtowc(&character, (const char*)&sequence[length - 1], 1, &reader->state);
        if (result == (size_t)-2)
            continue;
        if (result == (size_t)-1)
            return reject_sequence(reader, sequence, length, invalid_byte);
        return character;
    }
}
