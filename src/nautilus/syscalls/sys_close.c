#include <nautilus/fs.h>
#include <nautilus/syscalls/proc.h>

#define SYSCALL_NAME "sys_close"
#include "impl_preamble.h"

uint64_t sys_close(fd_t fd) {
  if (fd < 3) {
    DEBUG("Closing stdio is not defined\n");
  }

  nk_process_t* current_process = syscall_get_proc();

  struct nk_fs_open_file_state* nk_file = fd_to_nk_fs(&current_process->syscall_state.fd_table, fd);

  if (!nk_file) {
    DEBUG("Can't close an unopened file\n");
    return -1;
  }

  nk_fs_close(nk_file);
  
  return 0;
}
