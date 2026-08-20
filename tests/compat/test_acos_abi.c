#include <dlfcn.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef double (*acos_function)(double value);
typedef float (*acosf_function)(float value);

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "acos ABI test failed at line %d\n", __LINE__); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

static int close_to(double actual, double expected) {
    return fabs(actual - expected) <= 1e-14;
}

static int verify_value(acos_function function, double input, double expected) {
    errno = EDOM;
    CHECK(close_to(function(input), expected));
    CHECK(errno == EDOM);
    return 0;
}

static int close_to_float(float actual, float expected) {
    return fabsf(actual - expected) <= 2e-6f;
}

static int verify_float_value(acosf_function function, float input, float expected) {
    errno = EDOM;
    CHECK(close_to_float(function(input), expected));
    CHECK(errno == EDOM);
    return 0;
}

int main(void) {
    static const double pi = 3.141592653589793238462643383279502884;
    static const float float_pi = 3.141592653589793238462643383279502884f;
    acos_function function = acos;
    acosf_function float_function = acosf;
    double result;
    float float_result;
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "acos") == (void*)function);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)float_function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "acosf") == (void*)float_function);

    CHECK(verify_value(function, 1.0, 0.0) == 0);
    CHECK(verify_value(function, -1.0, pi) == 0);
    CHECK(verify_value(function, 0.0, pi / 2.0) == 0);
    CHECK(verify_value(function, -0.0, pi / 2.0) == 0);
    CHECK(verify_value(function, 0.5, pi / 3.0) == 0);
    CHECK(verify_value(function, -0.5, 2.0 * pi / 3.0) == 0);
    CHECK(verify_float_value(float_function, 1.0f, 0.0f) == 0);
    CHECK(verify_float_value(float_function, -1.0f, float_pi) == 0);
    CHECK(verify_float_value(float_function, 0.0f, float_pi / 2.0f) == 0);
    CHECK(verify_float_value(float_function, -0.0f, float_pi / 2.0f) == 0);
    CHECK(verify_float_value(float_function, 0.5f, float_pi / 3.0f) == 0);
    CHECK(verify_float_value(float_function, -0.5f, 2.0f * float_pi / 3.0f) == 0);

    errno = ERANGE;
    result = function(nextafter(1.0, 0.0));
    CHECK(result > 0.0 && result < 1e-6);
    CHECK(errno == ERANGE);
    errno = E2BIG;
    result = function(nextafter(-1.0, 0.0));
    CHECK(result < pi && result > pi - 1e-6);
    CHECK(errno == E2BIG);
    errno = ERANGE;
    float_result = float_function(nextafterf(1.0f, 0.0f));
    CHECK(float_result > 0.0f && float_result < 1e-3f);
    CHECK(errno == ERANGE);
    errno = E2BIG;
    float_result = float_function(nextafterf(-1.0f, 0.0f));
    CHECK(float_result < float_pi && float_result > float_pi - 1e-3f);
    CHECK(errno == E2BIG);

    errno = ENOTTY;
    result = function(NAN);
    CHECK(isnan(result));
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    float_result = float_function(NAN);
    CHECK(isnan(float_result));
    CHECK(errno == ENOTTY);

    errno = 0;
    result = function(1.5);
    CHECK(isnan(result));
    CHECK(errno == 0);
    errno = 0;
    float_result = float_function(1.5f);
    CHECK(isnan(float_result));
    CHECK(errno == 0);
    errno = 0;
    result = function(-1.5);
    CHECK(isnan(result));
    CHECK(errno == 0);
    errno = 0;
    float_result = float_function(-1.5f);
    CHECK(isnan(float_result));
    CHECK(errno == 0);
    errno = 0;
    result = function(INFINITY);
    CHECK(isnan(result));
    CHECK(errno == 0);
    errno = 0;
    float_result = float_function(INFINITY);
    CHECK(isnan(float_result));
    CHECK(errno == 0);
    return 0;
}
