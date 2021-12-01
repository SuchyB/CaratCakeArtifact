/* C/Fortran interface is different on different machines. 
 * You may need to tweak this.
 */


#ifndef _WTIME_H_
#define _WTIME_H_

#if defined(IBM)
#define wtime wtime
#elif defined(CRAY)
#define wtime WTIME
#else
#define wtime wtime_
#endif



#define time_t      unsigned
#define suseconds_t unsigned
struct timeval {
    time_t      tv_sec;     /* seconds */
    suseconds_t tv_usec;    /* microseconds */
};

struct timezone;

static inline int gettimeofdayEDIT(struct timeval *tv, struct timezone *tz_ignored){
  unsigned long nk_sched_get_realtime();
  unsigned long ns = nk_sched_get_realtime();

  tv->tv_sec = (time_t) (ns / 1000000000UL);
  tv->tv_usec = (suseconds_t) ((ns % 1000000000UL) / 1000UL);
  return 0;
}




#endif
