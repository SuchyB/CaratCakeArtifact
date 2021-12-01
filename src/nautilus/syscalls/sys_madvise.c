#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_madvise"
#include "impl_preamble.h"

uint64_t sys_madvise(uint64_t start, uint64_t len_in, uint64_t behavior) {
  DEBUG("Thanks for the advice, but we will ignore it :)\n");
  return 0;
}
