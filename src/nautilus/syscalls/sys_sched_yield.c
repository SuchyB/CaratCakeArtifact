#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_yield"
#include "impl_preamble.h"

uint64_t sys_sched_yield() {
  nk_yield();
  return 0;
}