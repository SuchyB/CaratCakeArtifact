#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_mprotect"
#include "impl_preamble.h"

uint64_t sys_mprotect(int addr, int length, int prot) {
  // "Works" by mmap setting liberal permissions
  DEBUG("This syscall does nothing\n");
  return 0;
}