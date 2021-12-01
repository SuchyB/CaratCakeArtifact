#include <nautilus/nautilus.h>
#include <nautilus/syscalls/decl.h>

#define SYSCALL_NAME "sys_writev"
#include "impl_preamble.h"

struct iovec {
  void* iov_base; /* Starting address */
  size_t iov_len; /* Number of bytes to transfer */
};

uint64_t sys_writev(uint64_t fd_arg, uint64_t vec_arg, uint64_t vlen_arg) {
  int fd = fd_arg;
  const struct iovec* iov = (const struct iovec*)vec_arg;
  int iovcnt = vlen_arg;

  DEBUG("Call to sys_writev with args: fd:%ld iov:%p iovlen:%ld\n", fd_arg,
        vec_arg, vlen_arg);

  uint64_t written_bytes = 0;
  for (int i = 0; i < iovcnt; i++) {
    written_bytes += sys_write(fd, iov[i].iov_base, iov[i].iov_len);
  }
  return written_bytes;
}