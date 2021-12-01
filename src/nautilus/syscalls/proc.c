#include <nautilus/syscalls/proc.h>

#define SYSCALL_NAME "proc"
#include "impl_preamble.h"

struct nk_fs_open_file_state* fd_to_nk_fs(struct file_descriptor_table* table,
                                          fd_t fd) {
  for (size_t i = 0; i < SYSCALL_NUM_FDS; i++) {
    if (table->fds[i].fd_no == fd) {
      return table->fds[i].nk_fs_ptr;
    }
  }
  return NULL;
}

fd_t fd_add(struct file_descriptor_table* table,
            struct nk_fs_open_file_state* nk_fd) {

  SYSCALL_ASSERT(
      table->next_fd_index < SYSCALL_NUM_FDS,
      "Can't support more fds, please add a better data structure.\n");

  struct file_descriptor* fd_table_entry = &table->fds[table->next_fd_index];

  fd_table_entry->fd_no =
      table->next_fd_index + 3; // offset for stdio fds, which are implicit
  fd_table_entry->nk_fs_ptr = nk_fd;

  table->next_fd_index += 1;

  return fd_table_entry->fd_no;
}

void free_process_syscall_state(struct nk_process_linux_syscall_state* syscall_state) {
  spin_lock(&syscall_state->lock);

  // Free all mmap'd regions
  struct linux_mmap_allocation* allocation =
      syscall_state->mmap_allocation_list;
  while (allocation) {
    struct linux_mmap_region* region = allocation->region_list;
    while (region) {
      struct linux_mmap_region* region_to_free = region;
      region = region->next;
      free(region_to_free);
    }
    struct linux_mmap_allocation* allocation_to_free = allocation;
    allocation = allocation->next;
    free(allocation_to_free->start);
    free(allocation_to_free);
  }

  // We don't want to release the state lock since implicitly using the state
  // after freeing it is a bug.
}

static void proc_mmap_print_allocations(nk_process_t* proc) {
  struct nk_process_linux_syscall_state* syscall_state = &proc->syscall_state;

  struct linux_mmap_allocation* allocation =
      syscall_state->mmap_allocation_list;
  if (allocation) {
    DEBUG("Process allocations now:\n");
  } else {
    DEBUG("No process allocations now!\n");
  }
  while (allocation) {
    DEBUG("  Allocation from 0x%p to 0x%p with regions:\n", allocation->start,
          allocation->end);
    struct linux_mmap_region* region = allocation->region_list;
    while (region) {
      DEBUG("    [0x%p, 0x%p]\n");
      region = region->next;
    }
    allocation = allocation->next;
  }
}

__attribute__((used, annotate("nocarat")))
void proc_mmap_add_region(nk_process_t* proc,
                          nk_aspace_region_t* aspace_region) {
  // This part is easy; just add the region to our tracking

  struct nk_process_linux_syscall_state* syscall_state = &proc->syscall_state;

  SYSCALL_ASSERT(aspace_region->pa_start == aspace_region->va_start,
                 "Design assumes identity mapping. pa_start=%p; va_start=%p\n",
                 aspace_region->pa_start, aspace_region->va_start);

  spin_lock(&syscall_state->lock);

  // Set up new allocation
  struct linux_mmap_allocation* old_head = syscall_state->mmap_allocation_list;
  struct linux_mmap_allocation* new_head =
      malloc(sizeof(struct linux_mmap_allocation));
  memset(new_head, 0, sizeof(struct linux_mmap_allocation));
  new_head->start = aspace_region->va_start;
  new_head->end = new_head->start + aspace_region->len_bytes;
  new_head->next = old_head;
  syscall_state->mmap_allocation_list = new_head;

  // Default region, which covers an entire allocation
  struct linux_mmap_region* default_region =
      malloc(sizeof(struct linux_mmap_region));
  memset(default_region, 0, sizeof(struct linux_mmap_region));
  default_region->start = new_head->start;
  default_region->end = new_head->end;

  new_head->region_list = default_region;

#ifdef NAUT_CONFIG_DEBUG_LINUX_SYSCALLS
  proc_mmap_print_allocations(proc);
#endif

  spin_unlock(&syscall_state->lock);
}

void proc_mmap_remove_region(nk_process_t* proc, void* start_addr,
                             uint64_t len) {
  void* end_addr = start_addr + len;

  SYSCALL_ASSERT(start_addr < end_addr, "Range to remove is not well-formed\n");

  struct nk_process_linux_syscall_state* syscall_state = &proc->syscall_state;

  spin_lock(&syscall_state->lock);

  // Iterate over all allocations and make holes in their region lists, freeing
  // the allocation if it contains no un-removed regions.
  struct linux_mmap_allocation* last_allocation = NULL;
  struct linux_mmap_allocation* allocation =
      syscall_state->mmap_allocation_list;
  while (allocation) {
    SYSCALL_ASSERT(allocation->start < allocation->end,
                   "mmap allocation is not well-formed\n");
    if (start_addr >= allocation->start && start_addr < allocation->end) {
      // This is the correct allocation to remove from.
      struct linux_mmap_region* last_region = NULL;
      struct linux_mmap_region* region = allocation->region_list;
      while (region) {
        SYSCALL_ASSERT(region->start < region->end,
                       "mmap region is not well-formed\n");
        if (region->start < start_addr && start_addr < region->end) {
          // Range starts in this region; trim the end of this region
          region->end = start_addr;
        } else if (region->start < end_addr && end_addr < region->end) {
          // Range ends in this region; trim the beginning of this region
          region->start = end_addr;
        } else if (start_addr <= region->start && region->end <= end_addr) {
          // This region is eclipsed by the range to remove, delete it
          if (last_region) {
            // Not the first region
            last_region->next = region->next;
          } else {
            // The first region
            allocation->region_list = region->next;
          }
          struct linux_mmap_region* region_to_free = region;
          region = region->next;
          free(region_to_free);
          continue;
        } else {
          SYSCALL_ASSERT(false, "This should not be possible.\n");
        }
        last_region = region;
        region = region->next;
      }
    }

    if (allocation->region_list == NULL) {
      // This allocation can be destroyed because it is now empty
      if (last_allocation) {
        // Not the first allocation
        last_allocation->next = allocation->next;
      } else {
        // The first allocation
        syscall_state->mmap_allocation_list = allocation->next;
      }
      struct linux_mmap_allocation* allocation_to_free = allocation;
      allocation = allocation->next;
      free(allocation_to_free->start);
      free(allocation_to_free);
      continue;
    }
    last_allocation = allocation;
    allocation = allocation->next;
  }

#ifdef NAUT_CONFIG_DEBUG_LINUX_SYSCALLS
  proc_mmap_print_allocations(proc);
#endif

  spin_unlock(&syscall_state->lock);
}
