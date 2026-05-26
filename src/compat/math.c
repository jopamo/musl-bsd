#include <math.h>

int __finite(double arg) {
    return isfinite(arg);
}

int __finitef(float arg) {
    return isfinite(arg);
}

int __finitel(long double arg) {
    return isfinite(arg);
}

int __isinf(double arg) {
    return isinf(arg);
}

int __isinff(float arg) {
    return isinf(arg);
}

int __isinfl(long double arg) {
    return isinf(arg);
}

int __isnan(double arg) {
    return isnan(arg);
}

int __isnanf(float arg) {
    return isnan(arg);
}

int __isnanl(long double arg) {
    return isnan(arg);
}
