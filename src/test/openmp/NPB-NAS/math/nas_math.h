
#ifndef _NAS_MATH_H_
#define _NAS_MATH_H_

#include <nautilus/nautilus.h>
#include "log/nas_log.h"
#include "pow/nas_pow.h"
#include "exp/nas_exp.h"
#include "sincos/nas_sincos.h"
//#define EXIT_FAILURE -1
//#define EXIT_SUCCESS 0

static inline double sqrt(double x)
{
    double ret;
    asm volatile(
		 "movq %1, %%xmm0 \n"
		 "sqrtsd %%xmm0, %%xmm1 \n"
		 "movq %%xmm1, %0 \n"
		 : "=r"(ret)
		 : "g"(x)
		 : "xmm0", "xmm1", "memory"
		 );
    return ret;
}

#define fabs(x) __builtin_fabs(x)
//#define pow(a,b) ___powl(a,b)

/* #if __HAVE_BUILTIN_TGMATH */

/* #  if __HAVE_FLOAT16 && __GLIBC_USE (IEC_60559_TYPES_EXT) */
/* #   define __TG_F16_ARG(X) X ## f16, */
/* #  else */
/* #   define __TG_F16_ARG(X) */
/* #  endif */
/* #  if __HAVE_FLOAT32 && __GLIBC_USE (IEC_60559_TYPES_EXT) */
/* #   define __TG_F32_ARG(X) X ## f32, */
/* #  else */
/* #   define __TG_F32_ARG(X) */
/* #  endif */
/* #  if __HAVE_FLOAT64 && __GLIBC_USE (IEC_60559_TYPES_EXT) */
/* #   define __TG_F64_ARG(X) X ## f64, */
/* #  else */
/* #   define __TG_F64_ARG(X) */
/* #  endif */
/* #  if __HAVE_FLOAT128 && __GLIBC_USE (IEC_60559_TYPES_EXT) */
/* #   define __TG_F128_ARG(X) X ## f128, */
/* #  else */
/* #   define __TG_F128_ARG(X) */
/* #  endif */
/* #  if __HAVE_FLOAT32X && __GLIBC_USE (IEC_60559_TYPES_EXT) */
/* #   define __TG_F32X_ARG(X) X ## f32x, */
/* #  else */
/* #   define __TG_F32X_ARG(X) */
/* #  endif */
/* #  if __HAVE_FLOAT64X && __GLIBC_USE (IEC_60559_TYPES_EXT) */
/* #   define __TG_F64X_ARG(X) X ## f64x, */
/* #  else */
/* #   define __TG_F64X_ARG(X) */
/* #  endif */
/* #  if __HAVE_FLOAT128X && __GLIBC_USE (IEC_60559_TYPES_EXT) */
/* #   define __TG_F128X_ARG(X) X ## f128x, */
/* #  else */
/* #  define __TG_F128X_ARG(X) */
/* #  endif */
/* #endif  */

/* #define __TGMATH_FUNCS(X) X ## f, X, X ## l,                          \ */
/*     __TG_F16_ARG (X) __TG_F32_ARG (X) __TG_F64_ARG (X) __TG_F128_ARG (X) \ */
/*     __TG_F32X_ARG (X) __TG_F64X_ARG (X) __TG_F128X_ARG (X) */

/* #define fabs(x) __builtin_fabs(x) */
/* #define __TGMATH_RCFUNCS(F, C) __TGMATH_FUNCS (F) __TGMATH_FUNCS (C) */
/* #define __TGMATH_2C(F, C, X, Y) __builtin_tgmath (__TGMATH_RCFUNCS (F, C) \ */
/*                                                     (X), (Y)) */
/* #define __TGMATH_BINARY_REAL_IMAG(Val1, Val2, Fct, Cfct)      \ */
/*   __TGMATH_2C (Fct, Cfct, (Val1), (Val2)) */

/* #define pow(Val1, Val2) __TGMATH_BINARY_REAL_IMAG (Val1, Val2, pow, cpow) */



#endif //COMMON_H
