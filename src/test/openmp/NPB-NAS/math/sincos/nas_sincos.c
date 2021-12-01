//extern int __branred(double x, double *a, double*aa);
#include "nas_type.h"
#include "nas_branred.h"
#include "__sincostab.h"
#include "nas_sincos.h"
#define ERROR(fmt, args...)
//ERROR_PRINT("omptest: " fmt, ##args)

#define fabs(x) __builtin_fabs(x)

//extern const union \
{ \
  int4 i[880]; \
  double x[440]; \
} __sincostab;




/* Helper macros to compute sin of the input values.  */
#define POLYNOMIAL2(xx) ((((s5 * (xx) + s4) * (xx) + s3) * (xx) + s2) * (xx))

#define POLYNOMIAL(xx) (POLYNOMIAL2 (xx) + s1)

/* The computed polynomial is a variation of the Taylor series expansion for
   sin(a):

   a - a^3/3! + a^5/5! - a^7/7! + a^9/9! + (1 - a^2) * da / 2

   The constants s1, s2, s3, etc. are pre-computed values of 1/3!, 1/5! and so
   on.  The result is returned to LHS.  */
double TAYLOR_SIN(double xx, double a,double da) {									      \
  double t = ((POLYNOMIAL (xx)  * (a) - 0.5 * (da))  * (xx) + (da));	      \
  double res = (a) + t;							      \
  return res;									      \
}

 void SINCOS_TABLE_LOOKUP(mynumber u, double* sn,double* ssn,double* cs, double *ccs){
	
  int4 k = u.i[LOW_HALF] << 2;
  *sn = __sincostab.x[k];						      \
  *ssn = __sincostab.x[k + 1];						      \
  *cs = __sincostab.x[k + 2];						      \
  *ccs = __sincostab.x[k + 3];						      \
}


static const double
  sn3 = -1.66666666666664880952546298448555E-01,
  sn5 = 8.33333214285722277379541354343671E-03,
  cs2 = 4.99999999999999999999950396842453E-01,
  cs4 = -4.16666666666664434524222570944589E-02,
  cs6 = 1.38888874007937613028114285595617E-03;


/* Given a number partitioned into X and DX, this function computes the cosine
   of the Taylor series) with the values looked up from the sin/cos table to
   get the result.  */
static inline double
//__always_inline
do_cos (double x, double dx)
{
  mynumber u;

  if (x < 0)
    dx = -dx;

  u.x = big + fabs (x);
  x = fabs (x) - (u.x - big) + dx;

  double xx=0, s=0, sn=0, ssn=0, c=0, cs=0, ccs=0, cor=0;
  xx = x * x;
  s = x + x * xx * (sn3 + xx * sn5);
  c = xx * (cs2 + xx * (cs4 + xx * cs6));
  SINCOS_TABLE_LOOKUP (u, &sn, &ssn, &cs, &ccs);
  cor = (ccs - s * ssn - cs * c) - sn * s;
  return cs + cor;
}

/* Given a number partitioned into X and DX, this function computes the sine of
   the number by combining the sin and cos of X (as computed by a variation of
   the Taylor series) with the values looked up from the sin/cos table to get
   the result.  */
static inline double
//__always_inline
do_sin (double x, double dx)
{
  double xold = x;
  /* Max ULP is 0.501 if |x| < 0.126, otherwise ULP is 0.518.  */
  if (fabs (x) < 0.126)
    return TAYLOR_SIN (x * x, x, dx);

  mynumber u;

  if (x <= 0)
    dx = -dx;
  u.x = big + fabs (x);
  x = fabs (x) - (u.x - big);

  double xx=0, s=0, sn=0, ssn=0, c=0, cs=0, ccs=0, cor=0;
  xx = x * x;
  s = x + (dx + x * xx * (sn3 + xx * sn5));
  c = x * dx + xx * (cs2 + xx * (cs4 + xx * cs6));
  SINCOS_TABLE_LOOKUP (u, &sn, &ssn,& cs, &ccs);
  cor = (ssn + s * ccs - sn * c) + cs * s;
  return __copysign (sn + cor, xold);
}

/* Reduce range of x to within PI/2 with abs (x) < 105414350.  The high part
   is written to *a, the low part to *da.  Range reduction is accurate to 136
   bits so that when x is large and *a very close to zero, all 53 bits of *a
   are correct.  */
static inline int4
//__always_inline
reduce_sincos (double x, double *a, double *da)
{
  mynumber v;

  double t = (x * hpinv + toint);
  //printf("t %20.20f hpinv %20.20f\n", t, x*hpinv);
  double xn = t - toint;
  //printf("x %20.20f xn %20.20f\n", x, xn);

  v.x = t;
  double y = (x - xn * mp1) - xn * mp2;
  int4 n = v.i[LOW_HALF] & 3;

  double b=0, db=0, t1=0, t2=0;
  t1 = xn * pp3;
  t2 = y - t1;
  //printf("t1 %20.20f t2 %20.20f\n", t1,t2);

  db = (y - t2) - t1;
  //printf("db %20.20f\n", db);

  t1 = xn * pp4;
  b = t2 - t1;
  //printf("b %20.20f\n", b);
  db += (t2 - b) - t1;
  //printf("db %20.20f\n", db);
  
  *a = b;
  *da = db;

  //printf("a %f, da %f return \n", *a, *da);
  return n;
}

/* Compute sin or cos (A + DA) for the given quadrant N.  */
static inline double
//__always_inline
do_sincos (double a, double da, int4 n)
{
  double retval;

  if (n & 1)
    /* Max ULP is 0.513.  */
    retval = do_cos (a, da);
  else
    /* Max ULP is 0.501 if xx < 0.01588, otherwise ULP is 0.518.  */
    retval = do_sin (a, da);

  return (n & 2) ? -retval : retval;
}


/*******************************************************************/
/* An ultimate sin routine. Given an IEEE double machine number x   */
/* it computes the correctly rounded (to nearest) value of sin(x)  */
/*******************************************************************/
double
__sin (double x)
{
  double t=0, a=0, da=0;
  mynumber u;
  int4 k=0, m=0, n=0;
  double retval = 0;

  //`SET_RESTORE_ROUND_53BIT (FE_TONEAREST);

  u.x = x;
  m = u.i[HIGH_HALF];
  k = 0x7fffffff & m;		/* no sign           */
  //printf("high half: %08x\n; low half %08x \n", k, u.i[LOW_HALF]);

  if (k < 0x3e500000)		/* if x->0 =>sin(x)=x */
    {
     ERROR("underflow !");
      //math_check_force_underflow (x);
      retval = x;
    }
/*--------------------------- 2^-26<|x|< 0.855469---------------------- */
  else if (k < 0x3feb6000)
    {
      /* Max ULP is 0.548.  */
      retval = do_sin (x, 0);
    }				/*   else  if (k < 0x3feb6000)    */

/*----------------------- 0.855469  <|x|<2.426265  ----------------------*/
  else if (k < 0x400368fd)
    {
      t = hp0 - fabs (x);
      /* Max ULP is 0.51.  */
      retval = __copysign (do_cos (t, hp1), x);
    }				/*   else  if (k < 0x400368fd)    */

/*-------------------------- 2.426265<|x|< 105414350 ----------------------*/
  else if (k < 0x419921FB)
    {
      n = reduce_sincos (x, &a, &da);
      //printf("n %20.20f\n a %20.20f, da %20.20f\n", n,a,da);
      retval = do_sincos (a, da, n);
    }				/*   else  if (k <  0x419921FB )    */

/* --------------------105414350 <|x| <2^1024------------------------------*/
  else if (k < 0x7ff00000)
    {
      n = __branred (x, &a, &da);
      retval = do_sincos (a, da, n);
    }
/*--------------------- |x| > 2^1024 ----------------------------------*/
  else
    {
      if (k == 0x7ff00000 && u.i[LOW_HALF] == 0)
//	__set_errno (EDOM);
      retval = x / x;
    }

  return retval;
}


/*******************************************************************/
/* An ultimate cos routine. Given an IEEE double machine number x   */
/* it computes the correctly rounded (to nearest) value of cos(x)  */
/*******************************************************************/

double
__cos (double x)
{
  double y=0, a=0, da=0;
  mynumber u;
  int4 k=0, m=0, n=0;

  double retval = 0;

  //SET_RESTORE_ROUND_53BIT (FE_TONEAREST);

  u.x = x;
  m = u.i[HIGH_HALF];
  k = 0x7fffffff & m;

  /* |x|<2^-27 => cos(x)=1 */
  if (k < 0x3e400000)
    retval = 1.0;

  else if (k < 0x3feb6000)
    {				/* 2^-27 < |x| < 0.855469 */
      /* Max ULP is 0.51.  */
      retval = do_cos (x, 0);
    }				/*   else  if (k < 0x3feb6000)    */

  else if (k < 0x400368fd)
    { /* 0.855469  <|x|<2.426265  */ ;
      y = hp0 - fabs (x);
      a = y + hp1;
      da = (y - a) + hp1;
      /* Max ULP is 0.501 if xx < 0.01588 or 0.518 otherwise.
	 Range reduction uses 106 bits here which is sufficient.  */
      retval = do_sin (a, da);
    }				/*   else  if (k < 0x400368fd)    */

  else if (k < 0x419921FB)
    {				/* 2.426265<|x|< 105414350 */
      n = reduce_sincos (x, &a, &da);
      retval = do_sincos (a, da, n + 1);
    }				/*   else  if (k <  0x419921FB )    */

  /* 105414350 <|x| <2^1024 */
  else if (k < 0x7ff00000)
    {
      n = __branred (x, &a, &da);
      retval = do_sincos (a, da, n + 1);
    }

  else
    {
      if (k == 0x7ff00000 && u.i[LOW_HALF] == 0)
	//__set_errno (EDOM);
      retval = x / x;		/* |x| > 2^1024 */
    }

  return retval;
}


double sin(double x) {return __sin(x);}
double cos(double x) {return __cos(x);}

