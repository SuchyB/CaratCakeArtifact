#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_sched_setaffinity"
#include "impl_preamble.h"

uint64_t sys_sched_setaffinity(uint64_t pid, uint64_t len,
                               uint64_t user_mask_ptr) {
  DEBUG("Args:%p\n%p\n%p\n", pid, len, user_mask_ptr);
  if (pid) {
    ERROR("Not supported for other processes\n");
    return -1;
  }

  if (!user_mask_ptr || *(uint64_t*)user_mask_ptr == 0) {
    // Doesn't make sense to set affinity to 0
    return -1;
  }

  // Set up the system bitmask (threads = num cpus), which is the max allowable
  // affinity
  static uint8_t calculated_system_bitmask = false;
  static uint64_t system_bitmask = 0;
  if (!calculated_system_bitmask) {
    for (int i = 0; i < nk_get_num_cpus(); i++) {
      system_bitmask |= 1 << i;
    }
    DEBUG("Set system bitmask %lx\n", system_bitmask);
    calculated_system_bitmask = true;
  }

  nk_thread_t* cur_thread = get_cur_thread();
  cur_thread->fake_affinity = *(uint64_t*)user_mask_ptr;
  cur_thread->fake_affinity &= system_bitmask;

  DEBUG("Set fake affinity to %lx\n", cur_thread->fake_affinity);

  return 0;
}
