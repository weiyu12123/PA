#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <common.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
    assert(-((int64_t)1 << 32) < ((int64_t) a * (int64_t) b) >> 16 &&
                   ((int64_t) a * (int64_t) b) >> 16 < ((int64_t)1 << 32));
    return ((int64_t) a * (int64_t) b) >> 16;
}


FLOAT F_div_F(FLOAT a, FLOAT b) {
    assert(b != 0);
    int sign = 1;
    if (a < 0) { sign = -sign; a = -a; }
    if (b < 0) { sign = -sign; b = -b; }

    // 1) 整数部分
    uint32_t rem = (uint32_t)a;
    uint32_t quot = rem / (uint32_t)b;
    rem = rem % (uint32_t)b;

    // 2) 构造定点：把整数部分左移16位
    quot <<= 16;

    // 3) 小数部分：迭代16次，每次产生1位
    for (int i = 0; i < 16; i++) {
        rem <<= 1;
        quot <<= 1;
        if (rem >= (uint32_t)b) {
            rem -= (uint32_t)b;
            quot |= 1;
        }
        // 如果需要调试精度，可以在此打印：
        printf("[F_div_F] step %d: quot=%u, rem=%u\n", i, quot, rem);
    }

    return sign < 0 ? -(FLOAT)quot : (FLOAT)quot;
}


/*FLOAT f2F(float a) {
  /* You should figure out how to convert `a' into FLOAT without
   * introducing x87 floating point instructions. Else you can
   * not run this code in NEMU before implementing x87 floating
   * point instructions, which is contrary to our expectation.
   *
   * Hint: The bit representation of `a' is already on the
   * stack. How do you retrieve it to another variable without
   * performing arithmetic operations on it directly?
   
  union float_ {
    struct {
      uint32_t m : 23;
      uint32_t e : 8;
      uint32_t signal : 1;
    };
    uint32_t value;
  };
  union float_ f;
  f.value = *((uint32_t*)(void*)&a);

  int e = f.e - 127;

  FLOAT result;
  if (e <= 7) {
    result = (f.m | (1 << 23)) >> 7 - e;
  }
  else {
    result = (f.m | (1 << 23)) << (e - 7);
  }
  return f.signal == 0 ? result : -result;
}*/

FLOAT f2F(float a) {
    union {
        float f;
        uint32_t u;
    } u = { .f = a };

    int sign = (u.u >> 31) & 1;
    int exp  = ((u.u >> 23) & 0xff) - 127;
    uint32_t frac = (u.u & 0x7fffff) | (1 << 23);

    int64_t val;
    if (exp >= 0) {
        // 左移 (exp) 位后，还要减去 7 位：23 位尾数对齐到 16 位小数
        val = (int64_t)frac << (exp - 7);
    } else {
        val = (int64_t)frac >> (7 - exp);
    }
    return sign ? (FLOAT)-val : (FLOAT)val;
}

/* Functions below are already implemented */

FLOAT Fabs(FLOAT a)
{
  return (a > 0) ? a : -a;
}

/*FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}



FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}*/

FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);
  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);

    // 调试输出：打印本轮迭代的 dt 和 t
    printf("[Fsqrt] dt = %d (%.6f), t_before = %d (%.6f)\n",
           dt, (float)dt / 65536.0, t, (float)t / 65536.0);

    t += dt;
  } while (Fabs(dt) > f2F(1e-4));

  return t;
}


FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);
  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;

    // 调试输出：打印本轮迭代的 dt 和 t
    printf("[Fpow ] dt = %d (%.6f), t_before = %d (%.6f)\n",
           dt, (float)dt / 65536.0, t, (float)t / 65536.0);

    t += dt;
  } while (Fabs(dt) > f2F(1e-4));

  return t;
}



