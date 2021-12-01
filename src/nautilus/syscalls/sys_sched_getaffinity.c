#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_sched_getaffinity"
#include "impl_preamble.h"

uint64_t sys_sched_getaffinity(uint64_t pid, uint64_t len,
                               uint64_t _user_mask_ptr) {
  DEBUG("Args:%p\n%p\n%p\n", pid, len, _user_mask_ptr);
  uint64_t* user_mask_ptr = (uint64_t*)_user_mask_ptr;

  // Set up the system bitmask (threads = num cpus), which is the default
  // affinity
  static uint8_t calculated_system_bitmask = false;
  static uint64_t system_bitmask = 0;
  if (!calculated_system_bitmask) {
    for (int i = 0; i < nk_get_num_cpus(); i++) {
      system_bitmask |= 1 << i;
    }
    calculated_system_bitmask = true;
  }

  uint64_t affinity_to_return;

  // PID !=0 means the caller is requesting info for another process/thread
  if (pid) {
    WARN("Not supported for other processes. Using fake return value\n");
    affinity_to_return = system_bitmask;
    goto out;
  }

  // Checks if the fake affinity was initialized or not,
  // since a process shouldn't set its affinity to 0
  nk_thread_t* cur_thread = get_cur_thread();
  if (cur_thread->fake_affinity == 0) {
    cur_thread->fake_affinity = system_bitmask;
  }

  affinity_to_return = cur_thread->fake_affinity;

out:
  if (len == 1) {
    *(uint8_t*)user_mask_ptr = affinity_to_return & 0xFF;
    return len;
  } else if (len == 2) {
    *(uint16_t*)user_mask_ptr = affinity_to_return & 0xFFFF;
    return len;
  } else if (len == 4) {
    *(uint32_t*)user_mask_ptr = affinity_to_return & 0xFFFFFFFF;
    return len;
  }
  *user_mask_ptr = affinity_to_return;
  DEBUG("Returning affinity %lx\n", *user_mask_ptr);
  return 8; /* Size of the bitmask data structure */
}