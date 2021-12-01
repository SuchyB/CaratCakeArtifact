#include <nautilus/nautilus.h>
#include <nautilus/syscalls/decl.h>

#define SYSCALL_NAME "sys_openat"
#include "impl_preamble.h"

uint64_t sys_openat(uint64_t dfd, uint64_t filename, uint64_t flags,
                    uint64_t mode) {
  /// TODO: entire syscall
  DEBUG("Call to fake syscall (openat)\nTry to open %s\n", filename);
  return sys_open(filename, flags, mode);
}