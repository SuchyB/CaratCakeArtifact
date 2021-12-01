typedef union{
double d;
unsigned long long u;
}udouble;

static double frexp(double x, int* e){
     udouble a;
     a.d = x;
     unsigned long long _a = a.u;

//inf
if( ! (~(_a<<1 >>53)<< 53)){
   *e = 1023;
   return 0;
}
if (x == 0.0){
  *e = 0;
  return 0;
}

udouble _man;
unsigned long long sign = _a >> 63 << 63, mask = 0x3FF;
_man.u = _a>>63<<63 \
         | mask << 52 \
	 | _a << 12 >> 12;

*e =  ((int)(_a << 1 >> 53) -1023 ) + 1;

//return _man.d/2;

//printf("man %lf  exp : %d \n", _man.d/2,  *e);

return _man.d/2;
}
