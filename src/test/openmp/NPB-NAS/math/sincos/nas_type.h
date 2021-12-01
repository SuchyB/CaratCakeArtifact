#ifndef _SINCOS_TYPE_H_
#define _SINCOS_TYPE_H_
#include "nas_mydefs.h"
//typedef int int4;
//typedef union { int4 i[2]; double x; } mynumber;

static const double s1 = -0x1.5555555555555p-3;   /* -0.16666666666666666     */
static const double s2 = 0x1.1111111110ECEp-7;    /*  0.0083333333333323288   */
static const double s3 = -0x1.A01A019DB08B8p-13;  /* -0.00019841269834414642  */
static const double s4 = 0x1.71DE27B9A7ED9p-19;   /*  2.755729806860771e-06   */
static const double s5 = -0x1.ADDFFC2FCDF59p-26;  /* -2.5022014848318398e-08  */
static const double aa = -0x1.5558000000000p-3;   /* -0.1666717529296875      */
static const double bb = 0x1.5555555556E24p-18;   /*  5.0862630208387126e-06  */
static const double big = 0x1.8000000000000p45;   /*  52776558133248          */
static const double hp0 = 0x1.921FB54442D18p0;    /*  1.5707963267948966      */
static const double hp1 = 0x1.1A62633145C07p-54;  /*  6.123233995736766e-17   */
static const double mp1 = 0x1.921FB58000000p0;    /*  1.5707963407039642      */
static const double mp2 = -0x1.DDE973C000000p-27; /* -1.3909067564377153e-08  */
static const double mp3 = -0x1.CB3B399D747F2p-55; /* -4.9789962505147994e-17  */
static const double pp3 = -0x1.CB3B398000000p-55; /* -4.9789962314799099e-17  */
static const double pp4 = -0x1.d747f23e32ed7p-83; /* -1.9034889620193266e-25  */
static const double hpinv = 0x1.45F306DC9C883p-1; /*  0.63661977236758138     */
static const double toint = 0x1.8000000000000p52; /*  6755399441055744        */
	
	
double
__copysign (double x, double y)
{
 union {double x; long long u;} _x, _y, _r;
 _x.x = x;
 _y.x = y;
 _r.u = _x.u <<1 >>1 | _y.u >>63 <<63;
 return _r.x;

  //return __builtin_copysign (x, y);
}


#   define LITTLE_ENDI 1
#   define HIGH_HALF 1
#   define  LOW_HALF 0

#endif
