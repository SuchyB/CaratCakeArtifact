#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/thread.h>

#define SYSCALL_NAME "sys_fork"
#include "impl_preamble.h"

uint64_t sys_fork() {
  DEBUG("Call to likely improperly-implemented syscall");
  return (uint64_t)nk_thread_fork();
}
