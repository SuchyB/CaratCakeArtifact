#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_mmap"
#include "impl_preamble.h"

#include <nautilus/syscalls/proc.h>

#define MAP_PRIVATE 0x2
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20
#define MAP_GROWSDOWN 0x100

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

__attribute__((used, annotate("nocarat")))
uint64_t sys_mmap(uint64_t addr_, uint64_t length, uint64_t prot_,
                  uint64_t flags_, uint64_t fd_, uint64_t offset_) {

  // Could also just edit the header to avoid these casts
  void* addr = (void*)addr_;
  int prot = (int)prot_;
  int flags = (int)flags_;
  int fd = (int)fd_;
  sint64_t offset = (sint64_t)offset_;

  if (fd != -1) {
    ERROR("Not implemented for file descriptors\n");
    return -1;
  }

  if (addr && flags & MAP_FIXED) {
    ERROR("Not implemented for fixed addresses\n");
    return -1;
  }

  if (!(flags & MAP_PRIVATE)) {
    ERROR("Not implemented for shared mappings\n");
    return -1;
  }

  if (!(flags & MAP_ANONYMOUS)) {
    ERROR("Not implemented for file-backed mappings\n");
    return -1;
  }

  if (flags & MAP_GROWSDOWN) {
    /* According to the man page for mmap, a region specified with this flag
     grows on page fault, so we can't currently support this in an
     aspace-agnostic way */
    ERROR("Nautilus is incompatible with this feature\n");
    return -1;
  }

  void* allocation = malloc(length);
  if (!allocation) {
    DEBUG("Malloc failed\n");
    return -1;
  }
  memset(allocation, 0, length); // mmap specifies 0-initialized memory

  nk_aspace_region_t* new_region = malloc(sizeof(nk_aspace_region_t));
  memset(new_region, 0, sizeof(nk_aspace_region_t));
  new_region->va_start = allocation;
  new_region->pa_start = allocation;
  new_region->len_bytes = length;
  // To simplify mprotect, grant all permissions for now. TODO
  new_region->protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE | NK_ASPACE_EXEC;

  // If the allocation is inside lower 4G, it is already mapped
  // if (allocation > (void*)0x100000000UL) {
  if (nk_aspace_add_region(nk_process_current()->aspace, new_region)) {
    DEBUG("Failed to add aspace region\n");
    free(allocation);
    return -1;
  }

  // Add the new region to the mmap tracking structure for this process.
  proc_mmap_add_region(syscall_get_proc(), new_region);

  return (uint64_t)allocation;
}