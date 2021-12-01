
#ifndef COMMON_H
#define COMMON_H

#include <nautilus/nautilus.h>

#define printf(...) nk_vc_printf(__VA_ARGS__)

#define atof(x) ((float)atoi(x))

#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0
#define exit(x) panic("exit from omptest\n");

#include <rt/openmp/openmp.h>
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

#endif //COMMON_H
