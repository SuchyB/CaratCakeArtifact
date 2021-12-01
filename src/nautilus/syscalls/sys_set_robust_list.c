#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_set_robust_list"
#include "impl_preamble.h"

uint64_t sys_set_robust_list(uint64_t header, uint64_t len) {
  DEBUG("Call to fake syscall (set_robust_list)\n");
  return 0;
}