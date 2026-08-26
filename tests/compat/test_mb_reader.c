#include <musl-bsd/mb-reader.h>

#include <errno.h>
#include <locale.h>
#include <stdio.h>

int main(void) {
    static const unsigned char input[] = {'A', 0xc3, '(', 0xe2, 0x82, 0xac, 0xff};
    struct musl_bsd_mb_reader reader;
    FILE* stream;
    int invalid;

    if (setlocale(LC_CTYPE, "C.UTF-8") == NULL)
        return 1;
    stream = tmpfile();
    if (stream == NULL)
        return 1;
    if (fwrite(input, 1, sizeof(input), stream) != sizeof(input))
        return 1;
    rewind(stream);
    musl_bsd_mb_reader_init(&reader, stream);

    if (musl_bsd_mb_reader_next(&reader, &invalid) != L'A' || invalid != -1)
        return 1;
    errno = 0;
    if (musl_bsd_mb_reader_next(&reader, NULL) != WEOF || errno != EILSEQ)
        return 1;
    errno = 0;
    if (musl_bsd_mb_reader_next(&reader, &invalid) != WEOF || errno != EILSEQ || invalid != 0xc3)
        return 1;
    if (musl_bsd_mb_reader_next(&reader, &invalid) != L'(' || invalid != -1)
        return 1;
    if (musl_bsd_mb_reader_next(&reader, &invalid) != L'\u20ac' || invalid != -1)
        return 1;
    errno = 0;
    if (musl_bsd_mb_reader_next(&reader, &invalid) != WEOF || errno != EILSEQ || invalid != 0xff)
        return 1;
    errno = 0;
    if (musl_bsd_mb_reader_next(&reader, &invalid) != WEOF || errno != 0 || invalid != -1)
        return 1;

    return fclose(stream) == 0 ? 0 : 1;
}
