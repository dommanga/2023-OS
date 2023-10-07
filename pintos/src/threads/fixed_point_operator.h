#include <stdint.h>

#define F (1 << 14)
#define INT_MAX ((1 << 31) - 1)
#define INT_MIN (-(1 << 31))

int int_to_fp (int n) {
  return n * F;
}

int fp_to_int_rd (int x) {
  return x / F;
}

int fp_to_int_rn (int x) {
  if (x >= 0) return (x + F / 2) / F;
  else return (x - F / 2) / F;
}

int fp_add (int x, int y) {
  return x + y;
}

int fp_sub (int x, int y) {
  return x - y;
}

int fp_int_add (int x, int n) {
  return x + n * F;
}

int fp_int_sub (int x, int n) {
  return x - n * F;
}

int fp_mul (int x, int y) {
  return ((int64_t) x) * y / F;
}

int fp_int_mul (int x, int n) {
  return x * n;
}

int fp_div (int x, int y) {
  return ((int64_t) x) * F / y;
}

int fp_int_div (int x, int n) {
  return x / n;
}