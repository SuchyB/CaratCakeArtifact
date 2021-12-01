#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_munmap"
#include "impl_preamble.h"

uint64_t sys_munmap(uint64_t addr, uint64_t length, int prot, int flags, int fd,
                    int offset) {
  proc_mmap_remove_region(syscall_get_proc(), addr, length);
  return 0;
}