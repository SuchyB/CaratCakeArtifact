#include <nautilus/nautilus.h>
#include <nautilus/timer.h>

#define SYSCALL_NAME "sys_nanosleep"
#include "impl_preamble.h"

/// TODO: move these to a better place
struct timespec {
  uint64_t tv_sec;  /* seconds */
  uint64_t tv_nsec; /* nanoseconds */
};
#define NSEC_PER_SEC (uint64_t)1000000000

uint64_t sys_nanosleep(uint64_t req, uint64_t rem) {
  const struct timespec* req_sleep = (struct timespec*)req;

  if (!req_sleep) {
    return -1;
  }

  nk_sleep(req_sleep->tv_sec * NSEC_PER_SEC + req_sleep->tv_nsec);

  return 0;
}