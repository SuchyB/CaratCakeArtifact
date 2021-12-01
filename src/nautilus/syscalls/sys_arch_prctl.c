#include <asm/prctl.h>
#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_arch_prctl"
#include "impl_preamble.h"

/// This is an x86-specific implementation
uint64_t sys_arch_prctl(uint64_t task, uint64_t option, uint64_t addrlen) {
  DEBUG("Task: %lx\n Option: %lx\n Addrlen %p\n", task, option, addrlen);

  // NOTE: the implementation here is based on reverse engineering of a specific
  // version of the c runtime. It is not consistent with either the linux kernel
  // or the manpage for arch_prctl! Further, this has not been extensively
  // tested and is most likely wrong in general.

  uint64_t ret = 0;
  switch (task) {
  case ARCH_SET_FS: {
    msr_write(MSR_FS_BASE, option);
    break;
  }
  case ARCH_SET_GS: {
    /*
     * Some changes need to be made to support changing gs, most notably:
     * 1. In the scheduler, have a concept that a user thread can change gs
     * 2. Swap gs on syscall entry/exit
     * 3. Swap gs on any call to Nautilus functions from the process
     */
    panic("Changing GS is not supported!\n");
    msr_write(MSR_GS_BASE, option);
    break;
  }
  case ARCH_GET_FS: {
    ret = msr_read(MSR_FS_BASE);
    break;
  }
  case ARCH_GET_GS: {
    ret = msr_read(MSR_GS_BASE);
    break;
  }
  default: {
    DEBUG("Unimplemented option\n");
    ret = -1;
  }
  }
  return ret;
}