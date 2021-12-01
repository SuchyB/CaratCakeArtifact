#include <nautilus/fs.h>
#include <nautilus/naut_types.h>
#include <nautilus/nautilus.h>

#define SYSCALL_NAME "sys_write"
#include "impl_preamble.h"

uint64_t sys_write(uint32_t fd, const char* buf, size_t len) {
  unsigned long flags;

  if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
    uint64_t i = 0;
    while (i < len) {
      nk_vc_putchar(*(char*)buf);
      buf++;
      i++;
    }
    return len;
  } else {
    nk_process_t* current_process = syscall_get_proc();
    struct nk_fs_open_file_state* nk_fd = fd_to_nk_fs(&current_process->syscall_state.fd_table, fd);
    return nk_fs_write(nk_fd, buf, len);
  }
}
