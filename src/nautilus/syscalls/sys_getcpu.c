#include <nautilus/nautilus.h>
#include <nautilus/percpu.h>

#define SYSCALL_NAME "sys_getcpu"
#include "impl_preamble.h"

uint64_t sys_getcpu(uint64_t cpu_, uint64_t node_, uint64_t unused) {
  unsigned* cpu = (unsigned*)cpu_;
  unsigned* node = (unsigned*)node_;

  *cpu = my_cpu_id();
  *node = 0;
  return 0;
}