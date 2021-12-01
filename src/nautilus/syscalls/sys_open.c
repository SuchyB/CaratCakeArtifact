#include <nautilus/fs.h>
#include <nautilus/nautilus.h>
#include <nautilus/syscalls/proc.h>

#define SYSCALL_NAME "sys_open"
#include "impl_preamble.h"

uint64_t sys_open(uint64_t filename, uint64_t flags, uint64_t mode) {
  DEBUG("open called with %p %p %p\n", filename, flags, mode);

  nk_process_t* current_process = syscall_get_proc();

  WARN("Ignoring flags passed to open syscall, granting RW/create permission\n");

  struct nk_fs_open_file_state* nk_file = nk_fs_open((char*)filename, O_RDWR | O_CREAT, 0);
  fd_t fd = SYSCALL_INVALID_FD;

  if (nk_file != FS_BAD_FD) {
    fd = fd_add(&current_process->syscall_state.fd_table, nk_file);
    if (fd == SYSCALL_INVALID_FD) {
      nk_fs_close(nk_file);
      DEBUG("Error adding fd to process state\n");
    }
  }
  return fd;
}
