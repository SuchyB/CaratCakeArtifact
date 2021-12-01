#include <nautilus/nautilus.h>
#include <sys/resource.h>

#define SYSCALL_NAME "sys_prlimit64"
#include "impl_preamble.h"

uint64_t sys_prlimit64(uint64_t pid, uint64_t resource, uint64_t new_rlim,
                       uint64_t _old_rlim) {
  struct rlimit* old_rlim = (struct rlimit*)_old_rlim;
  DEBUG("Called with args\n%p\n%p\n%p\n%p\n", pid, resource, new_rlim,
        old_rlim);
  switch (resource) {
  case RLIMIT_STACK: {
    old_rlim->rlim_cur = 8192 * 1024;
    old_rlim->rlim_max = RLIM_INFINITY;
    break;
  }
  default: {
    DEBUG("Unimplemented resource type\n");
    return -1;
  }
  }
  return 0;
}