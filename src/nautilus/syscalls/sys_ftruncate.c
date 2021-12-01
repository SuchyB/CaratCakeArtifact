#include <nautilus/fs.h>
#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_ftruncate"
#include "impl_preamble.h"

uint64_t sys_ftruncate(uint64_t fd, uint64_t len) {
  uint64_t ret;
  ret = nk_fs_ftruncate((struct nk_fs_open_file_state*)fd, (off_t)len);
  return ret;
}
