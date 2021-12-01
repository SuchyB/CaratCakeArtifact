#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_sigaction"
#include "impl_preamble.h"

uint64_t sys_rt_sigaction(uint64_t sig, uint64_t act, uint64_t oact,
                          uint64_t sigsetsize) {
  /// TODO: entire syscall
  DEBUG("Call to fake syscall (rt_sigaction)\n");
  return 0;
}

uint64_t sys_rt_sigprocmask(uint64_t how, uint64_t nset, uint64_t oset,
                            uint64_t sigsetsize) {
  /// TODO: entire syscall
  DEBUG("Call to fake syscall (rt_sigprocmask)\n");
  return 0;
}