#ifndef _NAS_LOG_H_
#define _NAS_LOG_H_
#include "frexp.h"
static double LOG2_HI = 0x1.62e42fee00000p-1;
static double LOG2_LOW = 0x1.a39ef35793c76p-33;
static double  L1 = 0x1.5555555555593p-1;
static double L2 = 0x1.999999997fa04p-2;
static double L3 = 0x1.2492494229359p-2;
static double L4 = 0x1.c71c51d8e78afp-3;
static double L5 = 0x1.7466496cb03dep-3;
static double L6 = 0x1.39a09d078c69fp-3;
static double L7 = 0x1.2f112df3e5244p-3;
static double SQRT2_HALF = 0x1.6a09e667f3bcdp-1;
static double fl, k, f, s, _s2,_s4,R,t1,t2,hfsq;

static double log(double x){

  int ki;
  double f1 = frexp(x,&ki);
 // printf("frac %f,exp %d\n", f1, ki);
  if (f1 < SQRT2_HALF){
     f1 *= 2;
     ki -= 1;
   }
   f = f1-1;
    k = (double)ki;

    s = f / (2 + f);
_s2 = s * s;
_s4 = _s2 * _s2;
// Terms with odd powers of s^2.
t1 = _s2 * (L1 + _s4 * (L3 + _s4 * (L5 + _s4 * L7)));
//Terms with even powers of s^2.
t2 = _s4 * (L2 + _s4 * (L4 + _s4 * L6));
R = t1 + t2;
hfsq = 0.5 * f * f;
return k * LOG2_HI - ((hfsq - (s * (hfsq + R) + k * LOG2_LOW)) - f);
}

//int main(){ \
  double input; \
  scanf("%lf", &input); \
  double lo = log(input); \
  printf("lo %f \n",lo); \
\
}

#endif
