#include <nautilus/nautilus.h>
#include <nautilus/thread.h>

#define SYSCALL_NAME "sys_getpid"
#include "impl_preamble.h"

/// TODO: are pids and tids the same?
uint64_t sys_getpid() {
  nk_thread_t* thread_id = get_cur_thread();
  uint64_t tid = thread_id->tid;
  return tid;
}
