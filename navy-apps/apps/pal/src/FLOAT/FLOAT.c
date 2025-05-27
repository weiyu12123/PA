#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>
#include <string.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
    assert(-((int64_t)1 << 32) < ((int64_t) a * (int64_t) b) >> 16 &&
                   ((int64_t) a * (int64_t) b) >> 16 < ((int64_t)1 << 32));
    return ((int64_t) a * (int64_t) b) >> 16;
}


FLOAT F_div_F(FLOAT a, FLOAT b) {
    int op = 1;
    if(a < 0) {
        op = -op;
        a = -a;
    }
    if(b < 0) {
        op = -op;
        b = -b;
    }
    int ret = a / b;
    a %= b;
    int i;
    for (i = 0;i < 16;i ++){
        a <<= 1;
        ret <<= 1;
        if (a >= b) a -= b, ret |= 1;
    }
    return op * ret;

}

FLOAT f2F(float a) {
  /* You should figure out how to convert `a' into FLOAT without
   * introducing x87 floating point instructions. Else you can
   * not run this code in NEMU before implementing x87 floating
   * point instructions, which is contrary to our expectation.
   *
   * Hint: The bit representation of `a' is already on the
   * stack. How do you retrieve it to another variable without
   * performing arithmetic operations on it directly?
   */
  union {
    float f;
    uint32_t i;
  } u;
  u.f = a;

  uint32_t sign = u.i >> 31;
  int32_t exp = ((u.i >> 23) & 0xff) - 127;
  uint32_t frac = (u.i & 0x7fffff) | 0x800000; // 1.xxx in IEEE754

  int32_t result;
  if (exp >= 7) {
    result = frac << (exp - 7);
  } else if (exp >= -16) {
    result = frac >> (7 - exp);
  } else {
    result = 0;  // underflow
  }

  return sign ? -result : result;
}

/* Functions below are already implemented */

FLOAT Fabs(FLOAT a) {
  return (a > 0) ? a : -a;
}


FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}
