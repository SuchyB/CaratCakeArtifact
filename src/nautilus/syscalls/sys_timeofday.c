/// At a file level, the utility functions here should probably be moved
/// elsewhere in Nautilus, and the syscalls should be split into their own files
/// for consistency

#include <dev/apic.h>
#include <nautilus/nautilus.h>

#include <errno.h>

#define SYSCALL_NAME "sys_timeofday"
#include "impl_preamble.h"

#define NSEC_PER_SEC 1000000000UL
#define NSEC_PER_USEC 1000UL

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

/// Offset applied when calculating the time from the cycle count
static uint64_t time_offset = 0;

/// TODO: move this struct definition somewhere useful and standard
struct timeval {
  uint64_t tv_sec;  // seconds
  uint64_t tv_usec; // microseconds
};

/// TODO: include from wherever this is defined correctly
struct timespec {
  uint64_t tv_sec;  /* seconds */
  uint64_t tv_nsec; /* nanoseconds */
};

/// TODO: add multiarch support if rdtsc is not a compiler intrinsic
/// @return The cycle count
uint64_t get_cycles() { return rdtsc(); }

/// @return Elapsed seconds since the cycle count was reset
uint64_t cycles2ns(uint64_t cycles) {
  struct sys_info* sys = per_cpu_get(system);
  struct apic_dev* apic = sys->cpus[my_cpu_id()]->apic;
  return apic_cycles_to_realtime(apic, cycles);
}

/// @return The time in nanoseconds
uint64_t get_time() { return cycles2ns(get_cycles()) + time_offset; }

/// @param ns The number of nanoseconds to offset against the 0 cycle count time
void set_time(uint64_t ns) { time_offset = ns - cycles2ns(get_cycles()); }

/// @param timeval_ptr struct timeval to mutate
/// @param timezone_ptr obsolete, not currently supported in Nautilus
uint64_t sys_gettimeofday(uint64_t timeval_ptr, uint64_t timezone_ptr) {
  struct timeval* tv = (struct timeval*)timeval_ptr;
  if (tv != NULL) {
    uint64_t now = get_time(); /* nanoseconds */

    tv->tv_sec = now / NSEC_PER_SEC;
    tv->tv_usec = (now % NSEC_PER_SEC) / NSEC_PER_USEC;

    ASSERT(tv->tv_usec < 1000000);

    DEBUG("Setting user time of day struct time to %d sec, %d usec\n",
          tv->tv_sec, tv->tv_usec);
  }
  /// TODO: add errno support?
  return 0;
}

/// @param timeval_ptr struct timeval to mutate
/// @param timezone_ptr obsolete, not currently supported in Nautilus
uint64_t sys_settimeofday(uint64_t timeval_ptr, uint64_t timezone_ptr) {
  /// TODO: add input validation and (maybe?) errno support
  /// (in Linux, tv_usec should not be greater than 999,999)
  struct timeval* tv = (struct timeval*)timeval_ptr;
  if (tv != NULL) {
    set_time((tv->tv_sec * NSEC_PER_SEC) + (tv->tv_usec * NSEC_PER_USEC));
  }
  return 0;
}

uint64_t sys_clock_gettime(uint64_t which_clock, uint64_t tp_) {
  struct timespec* tp = (struct timespec*)tp_;
  uint64_t when = get_time();

  if ((which_clock != CLOCK_REALTIME) && (which_clock != CLOCK_MONOTONIC))
    return -EINVAL;

  tp->tv_sec = when / NSEC_PER_SEC;
  tp->tv_nsec = when % NSEC_PER_SEC;

  return 0;
}

uint64_t sys_clock_getres(uint64_t clk_id, uint64_t tp_) {
  DEBUG_PRINT("Got to getres begin\n");
  struct timespec* tp = (struct timespec*)tp_;
  if (tp == NULL) {
    return 0;
  }
  DEBUG_PRINT("Got past getres pointer cast\n");
  tp->tv_sec = 0;
  DEBUG_PRINT("Got past getres set a value\n");
  tp->tv_nsec = 1000000000;

  DEBUG_PRINT("Got to end of getres\n");

  return 0;
}