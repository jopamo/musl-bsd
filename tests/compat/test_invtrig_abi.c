#include <dlfcn.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef double (*inverse_function)(double value);
typedef double (*inverse_binary_function)(double y, double x);
typedef float (*inverse_float_function)(float value);

#define CHECK(condition)                                                                     \
    do {                                                                                     \
        if (!(condition)) {                                                                  \
            fprintf(stderr, "inverse trigonometric ABI test failed at line %d\n", __LINE__); \
            return 1;                                                                        \
        }                                                                                    \
    } while (0)

static int close_to(double actual, double expected) {
    return fabs(actual - expected) <= 1e-14;
}

static int verify_value(inverse_function function, double input, double expected) {
    errno = EDOM;
    CHECK(close_to(function(input), expected));
    CHECK(errno == EDOM);
    return 0;
}

static int verify_binary_value(inverse_binary_function function, double y, double x, double expected) {
    errno = EDOM;
    CHECK(close_to(function(y, x), expected));
    CHECK(errno == EDOM);
    return 0;
}

static int close_to_float(float actual, float expected) {
    return fabsf(actual - expected) <= 2e-6f;
}

static int verify_float_value(inverse_float_function function, float input, float expected) {
    errno = EDOM;
    CHECK(close_to_float(function(input), expected));
    CHECK(errno == EDOM);
    return 0;
}

int main(void) {
    static const double pi = 3.141592653589793238462643383279502884;
    static const float float_pi = 3.141592653589793238462643383279502884f;
    inverse_function acos_function = acos;
    inverse_float_function acosf_function = acosf;
    inverse_function asin_function = asin;
    inverse_float_function asinf_function = asinf;
    inverse_function atan_function = atan;
    inverse_binary_function atan2_function = atan2;
    double result;
    float float_result;
    Dl_info info;

    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)acos_function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "acos") == (void*)acos_function);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)acosf_function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "acosf") == (void*)acosf_function);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)asin_function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "asin") == (void*)asin_function);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)asinf_function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "asinf") == (void*)asinf_function);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)atan_function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "atan") == (void*)atan_function);
    memset(&info, 0, sizeof(info));
    CHECK(dladdr((const void*)atan2_function, &info) != 0);
    CHECK(info.dli_fname != NULL);
    CHECK(strstr(info.dli_fname, "libmusl-bsd-core") == NULL);
    CHECK(dlsym(RTLD_DEFAULT, "atan2") == (void*)atan2_function);

    CHECK(verify_value(acos_function, 1.0, 0.0) == 0);
    CHECK(verify_value(acos_function, -1.0, pi) == 0);
    CHECK(verify_value(acos_function, 0.0, pi / 2.0) == 0);
    CHECK(verify_value(acos_function, -0.0, pi / 2.0) == 0);
    CHECK(verify_value(acos_function, 0.5, pi / 3.0) == 0);
    CHECK(verify_value(acos_function, -0.5, 2.0 * pi / 3.0) == 0);
    CHECK(verify_float_value(acosf_function, 1.0f, 0.0f) == 0);
    CHECK(verify_float_value(acosf_function, -1.0f, float_pi) == 0);
    CHECK(verify_float_value(acosf_function, 0.0f, float_pi / 2.0f) == 0);
    CHECK(verify_float_value(acosf_function, -0.0f, float_pi / 2.0f) == 0);
    CHECK(verify_float_value(acosf_function, 0.5f, float_pi / 3.0f) == 0);
    CHECK(verify_float_value(acosf_function, -0.5f, 2.0f * float_pi / 3.0f) == 0);
    CHECK(verify_value(asin_function, 1.0, pi / 2.0) == 0);
    CHECK(verify_value(asin_function, -1.0, -pi / 2.0) == 0);
    CHECK(verify_value(asin_function, 0.0, 0.0) == 0);
    CHECK(verify_value(asin_function, -0.0, -0.0) == 0);
    CHECK(signbit(asin_function(-0.0)) != 0);
    CHECK(verify_value(asin_function, 0.5, pi / 6.0) == 0);
    CHECK(verify_value(asin_function, -0.5, -pi / 6.0) == 0);
    CHECK(verify_float_value(asinf_function, 1.0f, float_pi / 2.0f) == 0);
    CHECK(verify_float_value(asinf_function, -1.0f, -float_pi / 2.0f) == 0);
    CHECK(verify_float_value(asinf_function, 0.0f, 0.0f) == 0);
    CHECK(verify_float_value(asinf_function, -0.0f, -0.0f) == 0);
    CHECK(signbit(asinf_function(-0.0f)) != 0);
    CHECK(verify_float_value(asinf_function, 0.5f, float_pi / 6.0f) == 0);
    CHECK(verify_float_value(asinf_function, -0.5f, -float_pi / 6.0f) == 0);
    CHECK(verify_value(atan_function, 0.0, 0.0) == 0);
    CHECK(verify_value(atan_function, -0.0, -0.0) == 0);
    CHECK(signbit(atan_function(-0.0)) != 0);
    CHECK(verify_value(atan_function, 1.0, pi / 4.0) == 0);
    CHECK(verify_value(atan_function, -1.0, -pi / 4.0) == 0);
    CHECK(verify_value(atan_function, 0.5, 0.4636476090008061162) == 0);
    CHECK(verify_value(atan_function, INFINITY, pi / 2.0) == 0);
    CHECK(verify_value(atan_function, -INFINITY, -pi / 2.0) == 0);
    CHECK(verify_binary_value(atan2_function, 0.0, 1.0, 0.0) == 0);
    CHECK(verify_binary_value(atan2_function, -0.0, 1.0, -0.0) == 0);
    CHECK(signbit(atan2_function(-0.0, 1.0)) != 0);
    CHECK(verify_binary_value(atan2_function, 0.0, -1.0, pi) == 0);
    CHECK(verify_binary_value(atan2_function, -0.0, -1.0, -pi) == 0);
    CHECK(verify_binary_value(atan2_function, 1.0, 0.0, pi / 2.0) == 0);
    CHECK(verify_binary_value(atan2_function, -1.0, 0.0, -pi / 2.0) == 0);
    CHECK(verify_binary_value(atan2_function, 1.0, 1.0, pi / 4.0) == 0);
    CHECK(verify_binary_value(atan2_function, -1.0, 1.0, -pi / 4.0) == 0);
    CHECK(verify_binary_value(atan2_function, 1.0, -1.0, 3.0 * pi / 4.0) == 0);
    CHECK(verify_binary_value(atan2_function, -1.0, -1.0, -3.0 * pi / 4.0) == 0);
    CHECK(verify_binary_value(atan2_function, INFINITY, INFINITY, pi / 4.0) == 0);
    CHECK(verify_binary_value(atan2_function, -INFINITY, INFINITY, -pi / 4.0) == 0);

    errno = ERANGE;
    result = acos_function(nextafter(1.0, 0.0));
    CHECK(result > 0.0 && result < 1e-6);
    CHECK(errno == ERANGE);
    errno = E2BIG;
    result = acos_function(nextafter(-1.0, 0.0));
    CHECK(result < pi && result > pi - 1e-6);
    CHECK(errno == E2BIG);
    errno = ERANGE;
    float_result = acosf_function(nextafterf(1.0f, 0.0f));
    CHECK(float_result > 0.0f && float_result < 1e-3f);
    CHECK(errno == ERANGE);
    errno = E2BIG;
    float_result = acosf_function(nextafterf(-1.0f, 0.0f));
    CHECK(float_result < float_pi && float_result > float_pi - 1e-3f);
    CHECK(errno == E2BIG);
    errno = ERANGE;
    result = asin_function(nextafter(1.0, 0.0));
    CHECK(result < pi / 2.0 && result > pi / 2.0 - 1e-6);
    CHECK(errno == ERANGE);
    errno = E2BIG;
    result = asin_function(nextafter(-1.0, 0.0));
    CHECK(result > -pi / 2.0 && result < -pi / 2.0 + 1e-6);
    CHECK(errno == E2BIG);
    errno = ERANGE;
    float_result = asinf_function(nextafterf(1.0f, 0.0f));
    CHECK(float_result < float_pi / 2.0f && float_result > float_pi / 2.0f - 1e-3f);
    CHECK(errno == ERANGE);
    errno = E2BIG;
    float_result = asinf_function(nextafterf(-1.0f, 0.0f));
    CHECK(float_result > -float_pi / 2.0f && float_result < -float_pi / 2.0f + 1e-3f);
    CHECK(errno == E2BIG);

    errno = ENOTTY;
    result = acos_function(NAN);
    CHECK(isnan(result));
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    float_result = acosf_function(NAN);
    CHECK(isnan(float_result));
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = asin_function(NAN);
    CHECK(isnan(result));
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    float_result = asinf_function(NAN);
    CHECK(isnan(float_result));
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = atan_function(NAN);
    CHECK(isnan(result));
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = atan2_function(NAN, 1.0);
    CHECK(isnan(result));
    CHECK(errno == ENOTTY);
    errno = ENOTTY;
    result = atan2_function(1.0, NAN);
    CHECK(isnan(result));
    CHECK(errno == ENOTTY);

    errno = 0;
    result = acos_function(1.5);
    CHECK(isnan(result));
    CHECK(errno == 0);
    errno = 0;
    float_result = acosf_function(1.5f);
    CHECK(isnan(float_result));
    CHECK(errno == 0);
    errno = 0;
    result = acos_function(-1.5);
    CHECK(isnan(result));
    CHECK(errno == 0);
    errno = 0;
    float_result = acosf_function(-1.5f);
    CHECK(isnan(float_result));
    CHECK(errno == 0);
    errno = 0;
    result = asin_function(1.5);
    CHECK(isnan(result));
    CHECK(errno == 0);
    errno = 0;
    float_result = asinf_function(1.5f);
    CHECK(isnan(float_result));
    CHECK(errno == 0);
    errno = 0;
    result = asin_function(-1.5);
    CHECK(isnan(result));
    CHECK(errno == 0);
    errno = 0;
    float_result = asinf_function(-1.5f);
    CHECK(isnan(float_result));
    CHECK(errno == 0);
    errno = 0;
    result = acos_function(INFINITY);
    CHECK(isnan(result));
    CHECK(errno == 0);
    errno = 0;
    float_result = acosf_function(INFINITY);
    CHECK(isnan(float_result));
    CHECK(errno == 0);
    errno = 0;
    result = asin_function(INFINITY);
    CHECK(isnan(result));
    CHECK(errno == 0);
    errno = 0;
    float_result = asinf_function(INFINITY);
    CHECK(isnan(float_result));
    CHECK(errno == 0);
    return 0;
}
