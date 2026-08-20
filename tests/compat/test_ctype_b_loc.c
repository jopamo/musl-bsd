#include <dlfcn.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

extern const unsigned short int** __ctype_b_loc(void);

/*
 * glibc exposes these masks in host byte order through __ctype_b_loc. On
 * little-endian x86_64, the network-order bit encoding has these values.
 */
enum {
    CTYPE_UPPER = 0x0100,
    CTYPE_LOWER = 0x0200,
    CTYPE_ALPHA = 0x0400,
    CTYPE_DIGIT = 0x0800,
    CTYPE_XDIGIT = 0x1000,
    CTYPE_SPACE = 0x2000,
    CTYPE_PRINT = 0x4000,
    CTYPE_GRAPH = 0x8000,
    CTYPE_BLANK = 0x0001,
    CTYPE_CNTRL = 0x0002,
    CTYPE_PUNCT = 0x0004,
    CTYPE_ALNUM = 0x0008,
};

#define CHECK(condition)                                                         \
    do {                                                                         \
        if (!(condition)) {                                                      \
            fprintf(stderr, "__ctype_b_loc test failed at line %d\n", __LINE__); \
            return 1;                                                            \
        }                                                                        \
    } while (0)

static unsigned short int ascii_mask(unsigned int character) {
    unsigned short int mask = 0;

    if (character <= 0x1f || character == 0x7f)
        mask |= CTYPE_CNTRL;
    if (character == '\t' || character == ' ')
        mask |= CTYPE_BLANK;
    if ((character >= '\t' && character <= '\r') || character == ' ')
        mask |= CTYPE_SPACE;
    if (character >= ' ' && character <= '~')
        mask |= CTYPE_PRINT;
    if (character >= '!' && character <= '~')
        mask |= CTYPE_GRAPH;

    if (character >= '0' && character <= '9')
        mask |= CTYPE_DIGIT | CTYPE_XDIGIT | CTYPE_ALNUM;
    else if (character >= 'A' && character <= 'Z') {
        mask |= CTYPE_UPPER | CTYPE_ALPHA | CTYPE_ALNUM;
        if (character <= 'F')
            mask |= CTYPE_XDIGIT;
    }
    else if (character >= 'a' && character <= 'z') {
        mask |= CTYPE_LOWER | CTYPE_ALPHA | CTYPE_ALNUM;
        if (character <= 'f')
            mask |= CTYPE_XDIGIT;
    }
    else if (character >= '!' && character <= '~') {
        mask |= CTYPE_PUNCT;
    }
    return mask;
}

static int verify_table(const unsigned short int* table) {
    int character;

    CHECK(table != NULL);
    for (character = -128; character < 0; ++character)
        CHECK(table[character] == 0);
    for (character = 0; character <= 0x7f; ++character)
        CHECK(table[character] == ascii_mask((unsigned int)character));
    for (character = 0x80; character <= 0xff; ++character)
        CHECK(table[character] == 0);
    return 0;
}

static int verify_locale(const char* name, int required, const unsigned short int** location) {
    locale_t locale;
    locale_t previous;

    locale = newlocale(LC_CTYPE_MASK, name, (locale_t)0);
    if (locale == (locale_t)0) {
        CHECK(!required);
        return 0;
    }

    previous = uselocale(locale);
    CHECK(previous != (locale_t)0);
    errno = EDOM;
    CHECK(__ctype_b_loc() == location);
    CHECK(errno == EDOM);
    CHECK(verify_table(*location) == 0);
    CHECK(uselocale(previous) != (locale_t)0);
    freelocale(locale);
    return 0;
}

int main(void) {
    const unsigned short int** (*ctype_b_loc)(void) = __ctype_b_loc;
    const unsigned short int** location;
    Dl_info info;

    errno = EDOM;
    location = ctype_b_loc();
    CHECK(location != NULL);
    CHECK(*location != NULL);
    CHECK(errno == EDOM);
    CHECK(ctype_b_loc() == location);
    CHECK(verify_table(*location) == 0);

    CHECK(verify_locale("C", 1, location) == 0);
    CHECK(verify_locale("C.UTF-8", 0, location) == 0);

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)ctype_b_loc, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "__ctype_b_loc") == (void*)ctype_b_loc);
    return 0;
}
