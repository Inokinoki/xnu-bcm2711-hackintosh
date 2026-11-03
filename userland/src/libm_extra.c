/*
 * What openlibm leaves to its libc / assumes about long double. On arm64 Darwin long double is the same type as
 * double, so the long double entry points are the double ones.
 */
#include <math.h>

double scalbn(double x, int n);

double ldexp(double x, int n) { return scalbn(x, n); }
long double ldexpl(long double x, int n) { return scalbn(x, n); }
long double scalbnl(long double x, int n) { return scalbn(x, n); }
