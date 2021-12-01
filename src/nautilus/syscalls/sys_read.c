#include <nautilus/fs.h>
#include <nautilus/naut_types.h>
#include <nautilus/syscalls/proc.h>

#define SYSCALL_NAME "sys_read"
#include "impl_preamble.h"

/// TODO: move macro to a unistd replacement
#define STDIN_FILENO 0

uint64_t sys_read(fd_t fd, uint64_t buf, uint64_t len) {
  int read_bytes;
  DEBUG("Called read with fd %d, buf %p, len %d\n", fd, buf, len);

  // First handle the special case where this is a read from STDIN
  if (fd == STDIN_FILENO) {
    int i = 0;
    char s;
    while (i < len) {
      *(char*)buf = nk_vc_getchar();
      i++;
      if (*(char*)buf == '\n') {
        return i;
      }
      buf++;
    }
    read_bytes = i;
    return read_bytes;
  }

  nk_process_t* current_process = syscall_get_proc();

  struct nk_fs_open_file_state* nk_fs_state = fd_to_nk_fs(&current_process->syscall_state.fd_table, fd);
  if (!nk_fs_state) {
    // File is not open
    return -1;
  }

  read_bytes = (int)nk_fs_read(nk_fs_state, (void*)buf,
                                 (ssize_t)len);
  return read_bytes;
}
