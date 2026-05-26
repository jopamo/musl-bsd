#include <time.h>

size_t __strftime_l(char* restrict s,
                    size_t n,
                    const char* restrict format,
                    const struct tm* restrict tm,
                    locale_t locale) {
    return strftime_l(s, n, format, tm, locale);
}
