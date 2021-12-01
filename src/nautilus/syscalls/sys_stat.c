#include <nautilus/fs.h>
#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_stat"
#include "impl_preamble.h"

uint64_t sys_stat(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e,
                  uint64_t f) {
  // TODO: add protection against bad fd
  char* pathname = (char*)a;
  struct nk_fs_stat* st = (struct nk_fs_stat*)b;
  uint64_t ret = nk_fs_stat(pathname, st);
  return ret;
}
